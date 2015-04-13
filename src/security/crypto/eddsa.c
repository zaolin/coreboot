/*
* This file is part of the coreboot project.
*
* Copyright (C) 2015 Philipp Deppenwiese <philipp@deppenwiese.net>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; version 2 of
* the License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <security/nacl.h>

int crypto_sign_open(u8 *m,u64 *mlen,const u8 *sm,u64 n,const u8 *pk)
{
  int i;
  u8 t[32],h[64];
  gf p[4],q[4];

  *mlen = -1;
  if (n < 64) return -1;

  if (unpackneg(q,pk)) return -1;

  FOR(i,n) m[i] = sm[i];
  FOR(i,32) m[i+32] = pk[i];
  crypto_hash(h,m,n);
  reduce(h);
  scalarmult(p,q,h);

  scalarbase(q,sm + 32);
  add(p,q);
  pack(t,p);

  n -= 64;
  if (crypto_verify_32(sm, t)) {
    FOR(i,n) m[i] = 0;
    return -1;
  }

  FOR(i,n) m[i] = sm[i + 64];
  *mlen = n;
  return 0;
}