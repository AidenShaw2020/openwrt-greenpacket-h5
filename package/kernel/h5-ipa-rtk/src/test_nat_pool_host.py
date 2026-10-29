#!/usr/bin/env python3
"""Compile the production pool against a host IPA DMA model (no device I/O).

Run on a Linux build host with gcc and ASan/UBSan. The model independently
traverses the hardware wire graphs after every insertion and deletion.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='h5-nat-test-') as tmp:
    d = Path(tmp)
    (d / 'linux/byteorder').mkdir(parents=True)
    for name in ('ipa_rtk_nat_pool.c', 'ipa_rtk_nat_pool.h', 'ipa_rtk_nat.h', 'ipa_rtk_nat_wire.h'):
        shutil.copyfile(root / name, d / name)
    for name in ('ipa_rtk_flow_manager.c', 'ipa_rtk_flow_manager.h'):
        shutil.copyfile(root / name, d / name)
    (d / 'shim.h').write_text(r'''
#ifndef SHIM_H
#define SHIM_H
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <endian.h>
#include <arpa/inet.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
typedef uint8_t __u8; typedef uint16_t __u16; typedef uint32_t __u32;
typedef uint16_t __be16; typedef uint32_t __be32; typedef uint32_t __le32;
typedef uintptr_t dma_addr_t;
#define cpu_to_le32(x) htole32(x)
#define le32_to_cpu(x) le32toh(x)
#define be32_to_cpu(x) be32toh(x)
#define be16_to_cpu(x) be16toh(x)
#define GFP_KERNEL 0
#define IPA_VERSION_5_0 50
#define IPA_MEM_PDN_CONFIG 1
struct mutex { int unused; };
#define DEFINE_MUTEX(n) struct mutex n
#define mutex_init(x) ((void)(x))
#define mutex_lock(x) ((void)(x))
#define mutex_unlock(x) ((void)(x))
#define dma_wmb() ((void)0)
#define dma_rmb() ((void)0)
#define READ_ONCE(x) (x)
#define memcpy_fromio memcpy
#define kvcalloc(n,s,f) calloc(n,s)
#define kvfree free
static void *dma_alloc_coherent(void *dev, size_t size, dma_addr_t *a, int flags)
{ (void)dev; (void)flags; void *p = calloc(1,size); *a = (uintptr_t)p; return p; }
static void dma_free_coherent(void *dev, size_t size, void *p, dma_addr_t a)
{ (void)dev; (void)size; (void)a; free(p); }
#define BITS (8*sizeof(unsigned long))
#define bitmap_zalloc(n,f) calloc(((n)+BITS-1)/BITS,sizeof(unsigned long))
#define bitmap_free free
static bool test_bit(unsigned n, unsigned long *b) {return (b[n/BITS]>>(n%BITS))&1;}
static void set_bit(unsigned n, unsigned long *b) {b[n/BITS]|=1UL<<(n%BITS);}
static void clear_bit(unsigned n, unsigned long *b) {b[n/BITS]&=~(1UL<<(n%BITS));}
static unsigned find_next_zero_bit(unsigned long *b,unsigned end,unsigned start)
{while(start<end && test_bit(start,b))start++;return start;}
#define find_first_zero_bit(b,n) find_next_zero_bit(b,n,0)
struct hlist_node {struct hlist_node *next,**pprev;};
struct hlist_head {struct hlist_node *first;};
static void hlist_add_head(struct hlist_node *n,struct hlist_head *h)
{n->next=h->first;if(n->next)n->next->pprev=&n->next;h->first=n;n->pprev=&h->first;}
static void hlist_del_init(struct hlist_node *n)
{if(n->pprev){*n->pprev=n->next;if(n->next)n->next->pprev=n->pprev;}n->next=NULL;n->pprev=NULL;}
#define container_of(p,t,m) ((t *)((char *)(p)-__builtin_offsetof(t,m)))
#define hlist_for_each_entry(p,h,m) \
 for(struct hlist_node *hn=(h)->first;hn && ((p)=container_of(hn,__typeof__(*(p)),m),1);hn=hn->next)
struct list_head {struct list_head *next,*prev;};
static void INIT_LIST_HEAD(struct list_head *h){h->next=h;h->prev=h;}
static bool list_empty(struct list_head *h){return h->next==h;}
static void list_add_tail(struct list_head *n,struct list_head *h)
{n->next=h;n->prev=h->prev;h->prev->next=n;h->prev=n;}
static void list_del_init(struct list_head *n)
{n->prev->next=n->next;n->next->prev=n->prev;INIT_LIST_HEAD(n);}
#define list_first_entry(h,t,m) container_of((h)->next,t,m)
static u32 jhash(const void *data,unsigned n,u32 seed)
{const u8 *b=data;for(unsigned i=0;i<n;i++)seed=(seed^b[i])*16777619;return seed;}
static u32 jhash_2words(u32 a,u32 b,u32 seed)
{u32 words[]={a,b};return jhash(words,sizeof(words),seed);}
struct ipa_mem { unsigned id, size, offset; };
struct ipa { int version; unsigned mem_count,mem_offset; const struct ipa_mem *mem;
 void *dev,*mem_virt; };
#endif
''')
    for name in ('mutex.h', 'dma-mapping.h', 'types.h', 'in.h', 'slab.h', 'bitmap.h', 'list.h', 'jhash.h', 'workqueue.h', 'byteorder/generic.h'):
        (d / 'linux' / name).write_text('#include "shim.h"\n')
    for name in ('ipa.h', 'ipa_mem.h'):
        (d / name).write_text('#include "shim.h"\n')
    (d / 'linux/jiffies.h').write_text('''#include "shim.h"
#define HZ 100
static unsigned long jiffies;
#define time_after(a,b) ((long)((b)-(a)) < 0)
#define time_after_eq(a,b) ((long)((a)-(b)) >= 0)
struct work_struct { int unused; };
struct delayed_work { struct work_struct work; };
#define system_wq ((void *)0)
#define to_delayed_work(w) container_of(w, struct delayed_work, work)
#define INIT_DELAYED_WORK(dw, fn) ((void)(dw), (void)(fn))
#define schedule_delayed_work(dw, delay) ((void)(dw), (void)(delay))
#define mod_delayed_work(wq, dw, delay) ((void)(wq), (void)(dw), (void)(delay))
#define cancel_delayed_work_sync(dw) ((void)(dw))
''')
    (d / 'ipa_rtk_flowtable.h').write_text('''#include "ipa_rtk_nat_wire.h"
#define ETH_HLEN 14
#define FLOW_ACTION_HW_STATS_DELAYED 1
struct ipa_rtk_flow { struct ipa_rtk_nat_tuple nat; int ingress_ifindex,egress_ifindex;
 bool destination; u8 ethernet[14]; };
struct flow_stats { unsigned long lastused; };
#define flow_stats_update(s,a,b,c,last,mode) ((s)->lastused=(last))
struct ipa_rtk_flowtable_ops {
 int (*replace)(void *,const void *,unsigned long,const struct ipa_rtk_flow *);
 int (*destroy)(void *,const void *,unsigned long);
 int (*stats)(void *,const void *,unsigned long,struct flow_stats *);
 void (*flush)(void *,const void *);
};
''')
    (d / 'test.c').write_text(r'''
#include "ipa_rtk_nat_pool.c"
static struct ipa_rtk_nat_layout hw;
static int fail_at, calls;
int ipa_cmd_rtk_nat_init(struct ipa *ipa,const struct ipa_rtk_nat_layout *l,dma_addr_t pdn)
{(void)ipa;(void)pdn;hw=*l;return 0;}
int ipa_cmd_rtk_nat_disable(struct ipa *ipa) {(void)ipa;return 0;}
int ipa_cmd_rtk_nat_dma(struct ipa *ipa,u8 table,u32 offset,u16 value)
{
 (void)ipa; if(fail_at && ++calls==fail_at)return -EIO;
 uintptr_t base[]={hw.base,hw.expansion,hw.index,hw.index_expansion};
 assert(table<4 && !(offset&1));
 assert(offset+2 <= (unsigned)(table&1 ? hw.expansion_count : hw.base_mask+1)*(table<2?32:4));
 uint16_t v=htole16(value); memcpy((void *)(base[table]+offset),&v,2);return 0;
}
#define N 4100
static struct ipa_rtk_nat_slot slots[N];
static struct ipa_rtk_nat_tuple tuples[N];
static u16 get16(uintptr_t base,unsigned index,unsigned size,unsigned field)
{u16 v;memcpy(&v,(void *)(base+index*size+field),2);return le16toh(v);}
static u16 rd(unsigned dir,u16 n,unsigned field)
{return get16(dir?hw.index:hw.base,n,dir?4:32,field);}
static bool lookup(const struct ipa_rtk_nat_tuple *t,unsigned dir,u16 expected)
{
 u16 n=dir?ipa_rtk_nat_source_hash(t,4095):ipa_rtk_nat_destination_hash(t,4095);
 for(unsigned steps=0;n;steps++) {
  assert(steps<4896 && n<4896);
  u16 d=dir?rd(1,n,0):n;
  if(!dir && !(rd(0,d,18)&0x8000))return false;
  if(dir && !d)return false;
  assert(d<4896);
  struct ipa_rtk_nat_entry *e=(void *)(hw.base+d*32);
  u32 w0=le32toh(e->word[0]),w1=le32toh(e->word[1]);
  u32 w2=le32toh(e->word[2]),w3=le32toh(e->word[3]);
  u8 proto=le32toh(e->word[5])>>24;
  if((rd(0,d,18)&0x8000) && proto==t->protocol && w1==ntohl(t->remote_ip) &&
    (w3>>16)==ntohs(t->remote_port) && (dir?
    (w0==ntohl(t->private_ip) && (u16)w3==ntohs(t->private_port)):
    ((w2>>16)==ntohs(t->public_port)))) {assert(d==expected);return true;}
  n=rd(dir,n,dir?2:8);
 }
 return false;
}
static void verify(struct ipa_rtk_nat_pool *p)
{
 unsigned count=0;
 for(unsigned i=0;i<N;i++)if(slots[i].installed) {
  count++;assert(lookup(&tuples[i],0,slots[i].destination));
  assert(lookup(&tuples[i],1,slots[i].destination));
 }
 assert(count==p->active_count);
}
static void make_tuple(unsigned i, bool collision)
{
 struct ipa_rtk_nat_tuple *t=&tuples[i];memset(t,0,sizeof(*t));
 t->public_ip=htonl(0x64400001);t->private_ip=htonl(0xc0a80000+(i%200)+1);
 t->remote_ip=htonl(0x08080000+i+1);t->protocol=17;
 t->remote_port=htons(443);t->private_port=htons(1000+i);t->public_port=htons(2000+i);
 if(collision) {
  /* Force both independent hashes into one bucket, without duplicate keys. */
  u32 r=ntohl(t->remote_ip),v=ntohl(t->private_ip),pub=ntohl(t->public_ip);
  t->private_port=htons((r^(r>>16)^v^(v>>16)^443^17^123)&4095);
  t->public_port=htons((r^(r>>16)^pub^(pub>>16)^443^17^321)&4095);
 }
}
int main(void)
{
 const struct ipa_mem mem={1,16,0};u8 pdn[16]={0};
 struct ipa ipa={.version=50,.mem_count=1,.mem=&mem,.mem_virt=pdn};
 struct ipa_rtk_nat_pool p={0};srand(918273);
 assert(!ipa_rtk_nat_pool_create(&p,&ipa,htonl(0x64400001),0,0));
 assert(p.layout.expansion_count==800 && p.total==4896);
 for(unsigned i=0;i<200;i++) {make_tuple(i,true);assert(!ipa_rtk_nat_pool_add(&p,&tuples[i],&slots[i]));verify(&p);}
 struct ipa_rtk_nat_slot duplicate={0};
 assert(ipa_rtk_nat_pool_add(&p,&tuples[0],&duplicate)==-EEXIST);
 /* Head, middle, tail, index promotion, sentinel reuse, and random churn. */
 assert(!ipa_rtk_nat_pool_delete(&p,&slots[0]));verify(&p);
 for(unsigned round=0;round<15000;round++) {
  unsigned i=rand()%500;
  if(slots[i].installed)assert(!ipa_rtk_nat_pool_delete(&p,&slots[i]));
  else {make_tuple(i,round<5000);int ret=ipa_rtk_nat_pool_add(&p,&tuples[i],&slots[i]);assert(ret==0||ret==-EEXIST);}
  verify(&p);
 }
 for(unsigned i=0;i<N;i++)if(slots[i].installed)assert(!ipa_rtk_nat_pool_delete(&p,&slots[i]));
 assert(!p.active_count);
 for(unsigned i=0;i<p.total;i++)assert(!test_bit(i,p.sources)&&!test_bit(i,p.destinations));
 /* All 800 expansion slots usable; a collision storm falls back cleanly. */
 for(unsigned i=0;i<801;i++){make_tuple(i,true);assert(!ipa_rtk_nat_pool_add(&p,&tuples[i],&slots[i]));}
 make_tuple(801,true);assert(ipa_rtk_nat_pool_add(&p,&tuples[801],&slots[801])==-ENOSPC);
 verify(&p);
 for(unsigned i=0;i<801;i++)assert(!ipa_rtk_nat_pool_delete(&p,&slots[i]));
 /* Exercise the configured logical cap with independent, collision-free keys. */
 for(unsigned i=0;i<4001;i++) {
  make_tuple(i,true);
  tuples[i].private_port=htons(ntohs(tuples[i].private_port)^123^(i+1));
  tuples[i].public_port=htons(ntohs(tuples[i].public_port)^321^(i+1));
  int ret=ipa_rtk_nat_pool_add(&p,&tuples[i],&slots[i]);
  assert(ret==(i==4000 ? -ENOSPC : 0));
 }
 verify(&p);
 for(unsigned i=0;i<4000;i++)assert(!ipa_rtk_nat_pool_delete(&p,&slots[i]));
 assert(!ipa_rtk_nat_pool_destroy(&p));
 /* Fail every publication command in a colliding insertion. */
 for(int step=1;step<=4;step++) {
  memset(&p,0,sizeof(p));memset(slots,0,sizeof(slots));
  assert(!ipa_rtk_nat_pool_create(&p,&ipa,htonl(0x64400001),0,0));
  make_tuple(0,true);make_tuple(1,true);
  assert(!ipa_rtk_nat_pool_add(&p,&tuples[0],&slots[0]));
  calls=0;fail_at=step;assert(ipa_rtk_nat_pool_add(&p,&tuples[1],&slots[1])==-EIO);
  assert(p.failed && !slots[1].installed);
  assert(ipa_rtk_nat_pool_delete(&p,&slots[0])==-ESHUTDOWN);
  fail_at=0;assert(!ipa_rtk_nat_pool_destroy(&p));
 }
 puts("PASS: production NAT pool; 200 forced-collision clients, 15000 churn operations, expansion exhaustion, DMA fault quarantine");
 return 0;
}
''')
    with (d / 'test.c').open('a') as f:
        f.write(r'''
#include "ipa_rtk_flow_manager.c"
static unsigned installs,removes,quiesces;
static int install(void *p,unsigned i,const struct ipa_rtk_flow f[2])
{(void)p;(void)i;(void)f;installs++;return 0;}
static int remove_flow(void *p,unsigned i){(void)p;(void)i;removes++;return 0;}
static void quiesce(void *p){(void)p;quiesces++;}
static void manager_test(void)
{
 struct ipa_rtk_flow_manager m={0};
 const struct ipa_rtk_flow_manager_ops ops={install,remove_flow,quiesce};
 const struct ipa_mem mem={1,16,0};u8 pdn[16]={0};
 struct ipa ipa={.version=50,.mem_count=1,.mem=&mem,.mem_virt=pdn};
 assert(!ipa_rtk_flow_manager_start(&m,&ipa,htonl(0x64400001),0,0,1,2,3,&ops,NULL));
 for(unsigned i=0;i<4000;i++) {
  make_tuple(i,true);
  tuples[i].private_port=htons(ntohs(tuples[i].private_port)^123^(i+1));
  tuples[i].public_port=htons(ntohs(tuples[i].public_port)^321^(i+1));
  struct ipa_rtk_flow f={.nat=tuples[i],.ingress_ifindex=1,.egress_ifindex=3};
  assert(managed_replace(&m,&ipa,2*i+1,&f)==-EAGAIN);
  f.destination=true;f.ingress_ifindex=3;f.egress_ifindex=1;
  assert(!managed_replace(&m,&ipa,2*i+2,&f));
 }
 assert(installs==4000 && m.pool.active_count==4000);
 for(unsigned i=4000;i>0;i--) {
  struct flow_stats stats={0};
  assert(!managed_stats(&m,&ipa,2*i,&stats));
 }
 int other_binding;
 assert(managed_destroy(&m,&other_binding,1)==-ENOENT);
 struct ipa_rtk_flow conflict={.nat=tuples[0],.ingress_ifindex=1,.egress_ifindex=3};
 conflict.nat.remote_ip^=htonl(1);
 assert(managed_replace(&m,&ipa,1,&conflict)==-ESTALE);
 conflict.nat=tuples[0];
 assert(managed_replace(&m,&ipa,99999,&conflict)==-EEXIST);
 for(unsigned i=0;i<4000;i++)assert(!managed_destroy(&m,&ipa,2*i+1));
 assert(removes==4000 && !m.pool.active_count && !quiesces);
 /* Verify TCP deferred teardown grace period and immediate reuse. */
 make_tuple(0,true);tuples[0].protocol=6;
 struct ipa_rtk_flow tf={.nat=tuples[0],.ingress_ifindex=1,.egress_ifindex=3};
 assert(managed_replace(&m,&ipa,50001,&tf)==-EAGAIN);
 tf.destination=true;tf.ingress_ifindex=3;tf.egress_ifindex=1;
 assert(!managed_replace(&m,&ipa,50002,&tf));
 assert(m.pool.active_count==1);
 assert(!managed_destroy(&m,&ipa,50001));
 assert(managed_destroy(&m,&ipa,50002)==0);
 assert(m.pool.active_count==1 && removes==4000);
 jiffies+=IPA_RTK_TEARDOWN_GRACE;
 retire_work_fn(&m.retire_work.work);
 assert(!m.pool.active_count && removes==4001);
 make_tuple(0,true);
 struct ipa_rtk_flow f={.nat=tuples[0],.ingress_ifindex=1,.egress_ifindex=3};
 assert(managed_replace(&m,&ipa,1,&f)==-EAGAIN);
 jiffies+=31*HZ;
 assert(managed_replace(&m,&ipa,2,&f)==-EAGAIN);
 unsigned pending=0;
 for(unsigned i=0;i<4000;i++)pending+=m.entries[i].used;
 assert(pending==1 && !m.pool.active_count);
 managed_flush(&m,&ipa);
 assert(managed_replace(&m,&ipa,3,&(struct ipa_rtk_flow){
  .nat=tuples[0],.ingress_ifindex=3,.egress_ifindex=1,.destination=true}) == -EAGAIN);
 managed_flush(&m,&ipa);
 assert(!ipa_rtk_flow_manager_stop(&m));
 managed_flush(&m,&ipa);
 assert(managed_destroy(&m,&ipa,2)==-ENOENT);
 assert(!m.entries);
 puts("PASS: production flow manager; 4000 paired flows, delete, pending expiry, callbacks after stop");
}
''')
    # Preserve the independent wire-graph test and add manager lifecycle tests.
    test = (d / 'test.c').read_text().replace('int main(void)', 'static int pool_test(void)')
    test += '\nint main(void) { pool_test(); manager_test(); return 0; }\n'
    (d / 'test.c').write_text(test)
    exe = d / 'test'
    subprocess.run(['gcc', '-D__KERNEL__', '-std=gnu11', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-g', '-O1', '-I', str(d),
                    str(d / 'test.c'), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
