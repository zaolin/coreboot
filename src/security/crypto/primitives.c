/*
* This file is part of the coreboot project.
*
*
*
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

u64 dl64(const u8 *x)
{
  u64 i,u=0;
  FOR(i,8) u=(u<<8)|x[i];
  return u;
}

void ts64(u8 *x,u64 u)
{
  int i;
  for (i = 7;i >= 0;--i) { x[i] = u; u >>= 8; }
}

static int vn(const u8 *x,const u8 *y,int n)
{
  u32 i,d = 0;
  FOR(i,n) d |= x[i]^y[i];
  return (1 & ((d - 1) >> 8)) - 1;
}

int crypto_verify_16(const u8 *x,const u8 *y)
{
  return vn(x,y,16);
}

int crypto_verify_32(const u8 *x,const u8 *y)
{
  return vn(x,y,32);
}

int crypto_verify_64(const u8 *x,const u8 *y)
{
  return vn(x,y,64);
}

static void set25519(gf r, const gf a)
{
  int i;
  FOR(i,16) r[i]=a[i];
}

static void car25519(gf o)
{
  int i;
  i64 c;
  FOR(i,16) {
    o[i]+=(1LL<<16);
    c=o[i]>>16;
    o[(i+1)*(i<15)]+=c-1+37*(c-1)*(i==15);
    o[i]-=c<<16;
  }
}

static void sel25519(gf p,gf q,int b)
{
  i64 t,i,c=~(b-1);
  FOR(i,16) {
    t= c&(p[i]^q[i]);
    p[i]^=t;
    q[i]^=t;
  }
}

static void pack25519(u8 *o,const gf n)
{
  int i,j,b;
  gf m,t;
  FOR(i,16) t[i]=n[i];
  car25519(t);
  car25519(t);
  car25519(t);
  FOR(j,2) {
    m[0]=t[0]-0xffed;
    for(i=1;i<15;i++) {
      m[i]=t[i]-0xffff-((m[i-1]>>16)&1);
      m[i-1]&=0xffff;
    }
    m[15]=t[15]-0x7fff-((m[14]>>16)&1);
    b=(m[15]>>16)&1;
    m[14]&=0xffff;
    sel25519(t,m,1-b);
  }
  FOR(i,16) {
    o[2*i]=t[i]&0xff;
    o[2*i+1]=t[i]>>8;
  }
}

static int neq25519(const gf a, const gf b)
{
  u8 c[32],d[32];
  pack25519(c,a);
  pack25519(d,b);
  return crypto_verify_32(c,d);
}

static u8 par25519(const gf a)
{
  u8 d[32];
  pack25519(d,a);
  return d[0]&1;
}

static void unpack25519(gf o, const u8 *n)
{
  int i;
  FOR(i,16) o[i]=n[2*i]+((i64)n[2*i+1]<<8);
  o[15]&=0x7fff;
}

static void A(gf o,const gf a,const gf b)
{
  int i;
  FOR(i,16) o[i]=a[i]+b[i];
}

static void Z(gf o,const gf a,const gf b)
{
  int i;
  FOR(i,16) o[i]=a[i]-b[i];
}

static void M(gf o,const gf a,const gf b)
{
  i64 i,j,t[31];
  FOR(i,31) t[i]=0;
  FOR(i,16) FOR(j,16) t[i+j]+=a[i]*b[j];
  FOR(i,15) t[i]+=38*t[i+16];
  FOR(i,16) o[i]=t[i];
  car25519(o);
  car25519(o);
}

static void S(gf o,const gf a)
{
  M(o,a,a);
}

static void inv25519(gf o,const gf i)
{
  gf c;
  int a;
  FOR(a,16) c[a]=i[a];
  for(a=253;a>=0;a--) {
    S(c,c);
    if(a!=2&&a!=4) M(c,c,i);
  }
  FOR(a,16) o[a]=c[a];
}

static void pow2523(gf o,const gf i)
{
  gf c;
  int a;
  FOR(a,16) c[a]=i[a];
  for(a=250;a>=0;a--) {
    S(c,c);
    if(a!=1) M(c,c,i);
  }
  FOR(a,16) o[a]=c[a];
}

int crypto_scalarmult(u8 *q,const u8 *n,const u8 *p)
{
  u8 z[32];
  i64 x[80],r,i;
  gf a,b,c,d,e,f;
  FOR(i,31) z[i]=n[i];
  z[31]=(n[31]&127)|64;
  z[0]&=248;
  unpack25519(x,p);
  FOR(i,16) {
    b[i]=x[i];
    d[i]=a[i]=c[i]=0;
  }
  a[0]=d[0]=1;
  for(i=254;i>=0;--i) {
    r=(z[i>>3]>>(i&7))&1;
    sel25519(a,b,r);
    sel25519(c,d,r);
    A(e,a,c);
    Z(a,a,c);
    A(c,b,d);
    Z(b,b,d);
    S(d,e);
    S(f,a);
    M(a,c,a);
    M(c,b,e);
    A(e,a,c);
    Z(a,a,c);
    S(b,a);
    Z(c,d,f);
    M(a,c,_121665);
    A(a,a,d);
    M(c,c,a);
    M(a,d,f);
    M(d,b,x);
    S(b,e);
    sel25519(a,b,r);
    sel25519(c,d,r);
  }
  FOR(i,16) {
    x[i+16]=a[i];
    x[i+32]=c[i];
    x[i+48]=b[i];
    x[i+64]=d[i];
  }
  inv25519(x+32,x+32);
  M(x+16,x+16,x+32);
  pack25519(q,x+16);
  return 0;
}

u64 R(u64 x,int c) { return (x >> c) | (x << (64 - c)); }
u64 Ch(u64 x,u64 y,u64 z) { return (x & y) ^ (~x & z); }
u64 Maj(u64 x,u64 y,u64 z) { return (x & y) ^ (x & z) ^ (y & z); }
u64 Sigma0(u64 x) { return R(x,28) ^ R(x,34) ^ R(x,39); }
u64 Sigma1(u64 x) { return R(x,14) ^ R(x,18) ^ R(x,41); }
u64 sigma0(u64 x) { return R(x, 1) ^ R(x, 8) ^ (x >> 7); }
u64 sigma1(u64 x) { return R(x,19) ^ R(x,61) ^ (x >> 6); }

void add(gf p[4],gf q[4])
{
  gf a,b,c,d,t,e,f,g,h;

  Z(a, p[1], p[0]);
  Z(t, q[1], q[0]);
  M(a, a, t);
  A(b, p[0], p[1]);
  A(t, q[0], q[1]);
  M(b, b, t);
  M(c, p[3], q[3]);
  M(c, c, D2);
  M(d, p[2], q[2]);
  A(d, d, d);
  Z(e, b, a);
  Z(f, d, c);
  A(g, d, c);
  A(h, b, a);

  M(p[0], e, f);
  M(p[1], h, g);
  M(p[2], g, f);
  M(p[3], e, h);
}

static void cswap(gf p[4],gf q[4],u8 b)
{
  int i;
  FOR(i,4)
    sel25519(p[i],q[i],b);
}

void pack(u8 *r,gf p[4])
{
  gf tx, ty, zi;
  inv25519(zi, p[2]);
  M(tx, p[0], zi);
  M(ty, p[1], zi);
  pack25519(r, ty);
  r[31] ^= par25519(tx) << 7;
}

void scalarmult(gf p[4],gf q[4],const u8 *s)
{
  int i;
  set25519(p[0],gf0);
  set25519(p[1],gf1);
  set25519(p[2],gf1);
  set25519(p[3],gf0);
  for (i = 255;i >= 0;--i) {
    u8 b = (s[i/8]>>(i&7))&1;
    cswap(p,q,b);
    add(q,p);
    add(p,p);
    cswap(p,q,b);
  }
}

void scalarbase(gf p[4],const u8 *s)
{
  gf q[4];
  set25519(q[0],X);
  set25519(q[1],Y);
  set25519(q[2],gf1);
  M(q[3],X,Y);
  scalarmult(p,q,s);
}

static void modL(u8 *r,i64 x[64])
{
  i64 carry,i,j;
  for (i = 63;i >= 32;--i) {
    carry = 0;
    for (j = i - 32;j < i - 12;++j) {
      x[j] += carry - 16 * x[i] * L[j - (i - 32)];
      carry = (x[j] + 128) >> 8;
      x[j] -= carry << 8;
    }
    x[j] += carry;
    x[i] = 0;
  }
  carry = 0;
  FOR(j,32) {
    x[j] += carry - (x[31] >> 4) * L[j];
    carry = x[j] >> 8;
    x[j] &= 255;
  }
  FOR(j,32) x[j] -= carry * L[j];
  FOR(i,32) {
    x[i+1] += x[i] >> 8;
    r[i] = x[i] & 255;
  }
}

void reduce(u8 *r)
{
  i64 x[64],i;
  FOR(i,64) x[i] = (u64) r[i];
  FOR(i,64) r[i] = 0;
  modL(r,x);
}

int unpackneg(gf r[4],const u8 p[32])
{
  gf t, chk, num, den, den2, den4, den6;
  set25519(r[2],gf1);
  unpack25519(r[1],p);
  S(num,r[1]);
  M(den,num,D);
  Z(num,num,r[2]);
  A(den,r[2],den);

  S(den2,den);
  S(den4,den2);
  M(den6,den4,den2);
  M(t,den6,num);
  M(t,t,den);

  pow2523(t,t);
  M(t,t,num);
  M(t,t,den);
  M(t,t,den);
  M(r[0],t,den);

  S(chk,r[0]);
  M(chk,chk,den);
  if (neq25519(chk, num)) M(r[0],r[0],I);

  S(chk,r[0]);
  M(chk,chk,den);
  if (neq25519(chk, num)) return -1;

  if (par25519(r[0]) == (p[31]>>7)) Z(r[0],gf0,r[0]);

  M(r[3],r[0],r[1]);
  return 0;
}