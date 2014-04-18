/*
 * This file is part of msrtool.
 *
 * Copyright (c) 2008 Peter Stuge <peter@stuge.se>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <pci/pci.h>

#include "msrtool.h"

#define DEFAULT_CPU 0
static uint8_t cpu = DEFAULT_CPU;

uint8_t targets_found = 0;
const struct targetdef **targets = NULL;
const struct sysdef *sys = NULL;
uint8_t reserved = 0, verbose = 0, quiet = 0;

struct pci_access *pacc = NULL;

static struct targetdef alltargets[] = {
	{ "geodegx2", "AMD Geode(tm) GX2", geodegx2_probe, geodegx2_msrs },
	{ "geodelx", "AMD Geode(tm) LX", geodelx_probe, geodelx_msrs },
	{ "cs5536", "AMD Geode(tm) CS5536", cs5536_probe, cs5536_msrs },
	{ "K8", "AMD K8 Family", k8_probe, k8_msrs },
	{ TARGET_EOT }
};

static struct sysdef allsystems[] = {
	{ "linux", "Linux with /dev/cpu/*/msr", linux_probe, linux_open, linux_close, linux_rdmsr },
	{ "darwin", "OS X with DirectIO", darwin_probe, darwin_open, darwin_close, darwin_rdmsr },
	{ "freebsd", "FreeBSD with /dev/cpuctl*", freebsd_probe, freebsd_open, freebsd_close, freebsd_rdmsr },
	{ SYSTEM_EOT }
};

static void syntax(char *argv[]) {
	printf("syntax: %s [-hvqrkl] [-c cpu] [-m system] [-t target ...]\n", argv[0]);
	printf("\t [-i addr=hi[:]lo] | [-s file] | [-d [:]file] | addr...\n");
	printf("  -h\t show this help text\n");
	printf("  -v\t be verbose\n");
	printf("  -q\t be quiet (overrides -v)\n");
	printf("  -r\t include [Reserved] values\n");
	printf("  -k\t list all known systems and targets\n");
	printf("  -l\t list MSRs and bit fields for current target(s) (-kl for ALL targets!)\n");
	printf("  -c\t access MSRs on the specified CPU, default=%d\n", DEFAULT_CPU);
	printf("  -m\t force a system, e.g: -m linux\n");
	printf("  -t\t force a target, can be used multiple times, e.g: -t geodelx -t cs5536\n");
	printf("  -i\t immediate mode\n");
	printf("\t decode hex addr=hi:lo for the target without reading hw value\n");
	printf("\t e.g: -i 4c00000f=f2f100ff56960004\n");
	printf("  -s\t stream mode\n");
	printf("\t read one MSR address per line and append current hw value to the line\n");
	printf("\t use the filename - for stdin/stdout\n");
	printf("\t using -l -s ignores input and will output all MSRs with values\n");
	printf("  -d\t diff mode\n");
	printf("\t read one address and value per line and compare with current hw value,\n");
	printf("\t printing differences to stdout. use the filename - to read from stdin\n");
	printf("\t use :file or :- to reverse diff, normally hw values are considered new\n");
	printf("  addr.. direct mode, read and decode values for the given MSR address(es)\n");
}

static void *add_target(const struct targetdef *t) {
	void *tmp;
	tmp = realloc(targets, (targets_found + 2) * sizeof(struct targetdef *));
	if (NULL == tmp) {
		perror("realloc");
		return tmp;
	}
	targets = tmp;
	targets[targets_found++] = t;
	targets[targets_found] = NULL;
	return targets;
}

int do_stream(const char *streamfn, uint8_t ignoreinput) {
	char tmpfn[20], line[256];
	uint8_t tn;
	size_t start, len;
	int ret = 1;
	int fdout = -1;
	FILE *fin = NULL, *fout = NULL;
	uint32_t addr, linenum;
	struct msr m = MSR1(0);

	if (0 == strcmp(streamfn, "-")) {
		fin = stdin;
		fout = stdout;
	} else {
		if (!ignoreinput) {
			if (NULL == (fin = fopen(streamfn, "r"))) {
				perror("fopen()");
				return 1;
			}
			if (snprintf(tmpfn, sizeof(tmpfn), "msrtoolXXXXXX") >= sizeof(tmpfn)) {
				perror("snprintf");
				return 1;
			}
			if (-1 == (fdout = mkstemp(tmpfn))) {
				perror("mkstemp");
				return 1;
			}
			if (NULL == (fout = fdopen(fdout, "w"))) {
				perror("fdopen");
				return 1;
			}
		} else {
			if (NULL == (fout = fopen(streamfn, "w"))) {
				perror("fopen");
				return 1;
			}
		}
	}

	if (!sys->open(cpu, SYS_RDONLY))
		goto done;
	if (ignoreinput) {
		for (tn = 0; tn < targets_found; tn++)
			if (dumpmsrdefsvals(fout, targets[tn], cpu)) {
				ret = 1;
				break;
			}
	} else {
		for (linenum = 1; NULL != fgets(line, sizeof(line), fin); ++linenum) {
			start = (0 == strncmp("0x", line, 2)) ? 2 : 0;
			if (1 == sscanf(line + start, "%8x", &addr)) {
				if (!sys->rdmsr(cpu, addr, &m))
					goto done;
				fprintf(fout, "0x%08x 0x%08x%08x\n", addr, m.hi, m.lo);
				continue;
			}
			while (1) {
				fprintf(fout, "%s", line);
				len = strlen(line);
				if (NULL != strchr("\r\n", line[len - 1]))
					break;
				if (NULL == fgets(line, sizeof(line), fin))
					goto read_done;
			}
		}
read_done:
		if (!feof(fin)) {
			fprintf(stderr, "%s:%d: fgets: %s\n", streamfn, linenum, strerror(errno));
			goto done;
		}
	}
	ret = 0;
done:
	sys->close(cpu);
	if (strcmp(streamfn, "-")) {
		if (ret)
			unlink(tmpfn);
		else if (!ignoreinput)
			rename(tmpfn, streamfn);
	}
	if (!ignoreinput)
		fclose(fin);
	fclose(fout);
	return ret;
}

int do_diff(const char *difffn) {
	char tmpfn[20], line[512], *m1start;
	size_t len;
	int ret = 1, tmp, m1pos;
	FILE *fin = NULL, *fout = stdout;
	uint8_t rev = 0;
	uint32_t addr, linenum;
	struct msr m1 = MSR1(0), m2 = MSR1(0);

	if (':' == difffn[0]) {
		rev = 1;
		++difffn;
	}
	if (0 == strcmp(difffn, "-"))
		fin = stdin;
	else if (NULL == (fin = fopen(difffn, "r"))) {
		perror("fopen()");
		return 1;
	}

	if (!sys->open(cpu, SYS_RDONLY))
		goto done;
	for (linenum = 1; NULL != fgets(line, sizeof(line), fin); ++linenum) {
		tmp = strncmp("0x", line, 2) ? 0 : 2;
		if (sscanf(line + tmp, "%8x %n%*x", &addr, &m1pos) < 1)
			continue;
		m1start = line + tmp + m1pos;
		for (len = strlen(m1start) - 1; NULL != strchr("\r\n", m1start[len]); --len)
			m1start[len] = 0;
		if (!str2msr(m1start, &m1)) {
			fprintf(stderr, "%s:%d: invalid MSR value '%s'\n", difffn, linenum, m1start);
			continue;
		}
		if (!sys->rdmsr(cpu, addr, &m2))
			goto done;
		if (diff_msr(fout, addr, rev ? m2 : m1, rev ? m1 : m2))
			fprintf(fout, "\n");
	}
	if (!feof(fin))
		fprintf(stderr, "%s:%d: fgets: %s\n", difffn, linenum, strerror(errno));
	else
		ret = 0;
done:
	sys->close(cpu);
	if (strcmp(difffn, "-")) {
		if (ret)
			unlink(tmpfn);
		else
			rename(tmpfn, difffn);
		fclose(fin);
		fclose(fout);
	}
	return ret;
}

int main(int argc, char *argv[]) {
	char c;
	int ret = 1;
	const struct sysdef *s;
	const struct targetdef *t;
	uint8_t tn, listmsrs = 0, listknown = 0, input = 0;
	uint32_t addr = 0;
	const char *streamfn = NULL, *difffn = NULL;
	struct msr msrval = MSR2(-1, -1);
	while ((c = getopt(argc, argv, "hqvrklc:m:t:a:i:s:d:")) != -1)
		switch (c) {
		case 'h':
			syntax(argv);
			return 0;
		case 'q':
			quiet = 1;
			break;
		case 'v':
			++verbose;
			break;
		case 'r':
			reserved = 1;
			break;
		case 'k':
			listknown = 1;
			break;
		case 'l':
			listmsrs = 1;
			break;
		case 'c':
			cpu = atoi(optarg);
			break;
		case 'm':
			for (s = allsystems; !SYSTEM_ISEOT(*s); s++)
				if (!strcmp(s->name, optarg)) {
					sys = s;
					break;
				}
			break;
		case 't':
			for (t = alltargets; !TARGET_ISEOT(*t); t++)
				if (!strcmp(t->name, optarg)) {
					add_target(t);
					break;
				}
			break;
		case 'i':
			input = 1;
			addr = msraddrbyname(optarg);
			optarg = strchr(optarg, '=');
			if (NULL == optarg) {
				fprintf(stderr, "missing value in -i argument!\n");
				break;
			}
			if (!str2msr(++optarg, &msrval))
				fprintf(stderr, "invalid value in -i argument!\n");
			break;
		case 's':
			streamfn = optarg;
			break;
		case 'd':
			difffn = optarg;
			break;
		default:
			break;
		}

	printf_quiet("msrtool %s\n", VERSION);

	pacc = pci_alloc();
	if (NULL == pacc) {
		fprintf(stderr, "Could not initialize PCI library! pci_alloc() failed.\n");
		return 1;
	}
	pci_init(pacc);
	pci_scan_bus(pacc);

	if (!sys && !input && !listknown)
		for (sys = allsystems; !SYSTEM_ISEOT(*sys); sys++) {
			printf_verbose("Probing for system %s: %s\n", sys->name, sys->prettyname);
			if (!sys->probe(sys))
				continue;
			printf_quiet("Detected system %s: %s\n", sys->name, sys->prettyname);
			break;
		}

	if (targets)
		for (tn = 0; tn < targets_found; tn++)
			printf_quiet("Forced target %s: %s\n", targets[tn]->name, targets[tn]->prettyname);
	else 
		for (t = alltargets; !TARGET_ISEOT(*t); t++) {
			printf_verbose("Probing for target %s: %s\n", t->name, t->prettyname);
			if (!t->probe(t))
				continue;
			printf_quiet("Detected target %s: %s\n", t->name, t->prettyname);
			add_target(t);
		}

	printf_quiet("\n");
	fflush(stdout);

	if (listknown) {
		printf("Known systems:\n");
		for (s = allsystems; s->name; s++)
			printf("%s: %s\n", s->name, s->prettyname);
		printf("\nKnown targets:\n");
		for (t = alltargets; t->name; t++) {
			if (listmsrs && alltargets != t)
				printf("\n");
			printf("%s: %s\n", t->name, t->prettyname);
			if (listmsrs)
				dumpmsrdefs(t);
		}
		printf("\n");
		return 0;
	}

	if (sys && !sys->name) {
		fprintf(stderr, "Unable to detect the current operating system!\n");
		fprintf(stderr, "On Linux, please do 'modprobe msr' and retry.\n");
		fprintf(stderr, "Please send a report or patch to coreboot@coreboot.org. Thanks for your help!\n");
		fprintf(stderr, "\n");
	}

	if (!targets_found || !targets) {
		fprintf(stderr, "Unable to detect a known target; can not decode any MSRs! (Use -t to force)\n");
		fprintf(stderr, "Please send a report or patch to coreboot@coreboot.org. Thanks for your help!\n");
		fprintf(stderr, "\n");
		return 1;
	}

	if (input) {
		decodemsr(cpu, addr, msrval);
		return 0;
	}

	if (sys && !sys->name)
		return 1;

	if (listmsrs) {
		if (streamfn)
			return do_stream(streamfn, 1);
		for (tn = 0; tn < targets_found; tn++) {
			if (tn)
				printf("\n");
			dumpmsrdefs(targets[tn]);
		}
		printf("\n");
		return 0;
	}

	if (streamfn)
		return do_stream(streamfn, 0);

	if (!sys->open(cpu, SYS_RDONLY))
		return 1;

	if (difffn) {
		ret = do_diff(difffn);
		goto done;
	}

	if (optind == argc) {
		syntax(argv);
		printf("\nNo mode or address(es) specified!\n");
		goto done;
	}

	for (; optind < argc; optind++) {
		addr = msraddrbyname(argv[optind]);
		if (!sys->rdmsr(cpu, addr, &msrval))
			break;
		decodemsr(cpu, addr, msrval);
	}
	ret = 0;
done:
	sys->close(cpu);
	return ret;
}
