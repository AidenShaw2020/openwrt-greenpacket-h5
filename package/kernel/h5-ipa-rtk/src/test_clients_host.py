#!/usr/bin/env python3
"""Test production client/route code with a DMA publication model, no device I/O."""
from pathlib import Path
import re
import subprocess
import tempfile
import shutil

root = Path(__file__).resolve().parent
source = (root / 'ipa_rtk_platform.c').read_text()
backend_file = root / 'ipa_table_rtk.inc'
if backend_file.exists():
    backend = backend_file.read_text()
else:
    patch = root.parents[3] / 'target/linux/sdx/patches-6.12/121-ipa-h5-system-header-contexts.patch'
    backend = '\n'.join(line[1:] for line in patch.read_text().splitlines()
                        if line.startswith('+') and not line.startswith('+++'))

def function(text, name):
    start = re.search(r'(?:static )?int ' + name + r'\(', text).start()
    brace = text.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end] + '\n'

with tempfile.TemporaryDirectory(prefix='h5-client-test-') as tmp:
    d = Path(tmp)
    for name in ('ipa_rtk_rule_wire.h', 'ipa_rtk_nat_wire.h'):
        shutil.copyfile(root / name, d / name)
    text = r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "ipa_rtk_rule_wire.h"
#include "ipa_rtk_nat_wire.h"
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
typedef uintptr_t dma_addr_t;
#define BIT(n) (1U<<(n))
#define U32_MAX UINT32_MAX
#define ETH_HLEN 14
#define IPA_RTK_HEADER_CAPACITY 200
#define IPA_RTK_MAX_HEADERS IPA_RTK_HEADER_CAPACITY
#define IPA_RTK_FLOW_LIMIT 4000
#define IPA_RTK_ROUTE_SIZE 8192
#define IPA_RTK_SYSTEM_CONTEXT_BASE 0x470
#define lower_32_bits(v) ((u32)(v))
#define dma_wmb() ((void)0)
static u32 readl(void *at) {u32 v;memcpy(&v,at,4);return v;}
struct ipa {void *reg_virt;};
struct ipa_rtk_header_bank {struct ipa *ipa;void *buffer;dma_addr_t address;};
struct ipa_rtk_table {unsigned unused;};
struct ipa_rtk_flow {struct ipa_rtk_nat_tuple nat;u8 ethernet[14];};
'''
    for name in ('ipa_rtk_header_ref', 'ipa_rtk_offload_slot', 'ipa_rtk_client'):
        text += re.search(r'struct ' + name + r' \{.*?\n\};', source, re.S)[0] + '\n'
    text += r'''
struct ipa_rtk_controller {
 struct ipa *ipa;
 struct ipa_rtk_header_ref headers[200];
 struct ipa_rtk_client clients[200];
 struct ipa_rtk_offload_slot *flows;
 struct ipa_rtk_header_bank header_bank;
 struct ipa_rtk_table route;
 void *offload_buf;
 dma_addr_t offload_addr;
 unsigned route_flip;
 uint64_t total_installs,total_removes;
};
static struct ipa_rtk_controller *current;
static unsigned publications;
static int fail_publish;
static dma_addr_t active_address;
static int ipa_table_rtk_replace(struct ipa_rtk_table *table,dma_addr_t address)
{
 assert(table==&current->route);
 assert(address!=active_address);
 if(fail_publish){fail_publish--;return -EIO;}
 u8 *p=(void *)address;unsigned count=0,expected=0;
 for(unsigned i=0;i<200;i++)expected+=!!current->clients[i].refcount;
 while(1) {
  uint64_t header;memcpy(&header,p,8);header=le64toh(header);
  if(!header)break;
  assert(++count<=200);
  assert(((header>>61)&1) && ((header>>63)&1));
  assert(((header>>16)&255)==26);
  unsigned slot=(header>>52)&511;assert(slot<200);
  assert(p[8]==16);
  u32 ip;memcpy(&ip,p+20,4);ip=le32toh(ip);
  bool found=false;
  for(unsigned i=0;i<200;i++)if(current->clients[i].refcount &&
    ntohl(current->clients[i].address)==ip && current->clients[i].header_index==slot)found=true;
  assert(found);
  u32 *ctx=current->header_bank.buffer+slot*32;
  assert(le32toh(ctx[0])==(1|2<<8|14<<16));
  assert(le32toh(ctx[1])==0x100000+200*32+slot*16);
  assert(!ctx[2] && !ctx[3]);
  assert(!memcmp(current->header_bank.buffer+200*32+slot*16,current->headers[slot].ethernet,14));
  p+=24;
 }
 assert(count==expected);
 active_address=address;publications++;return 0;
}
'''
    text += function(backend, 'ipa_header_rtk_bank_set')
    for name in ('ipa_rtk_header_get', 'ipa_rtk_header_put', 'ipa_rtk_rebuild_routes',
                 'ipa_rtk_flow_install', 'ipa_rtk_flow_remove'):
        text += function(source, name)
    text += r'''
static void make_flow(struct ipa_rtk_flow f[2],unsigned client)
{
 memset(f,0,2*sizeof(*f));f[1].nat.private_ip=htonl(0xc0000201+client);
 f[1].ethernet[0]=2;f[1].ethernet[4]=client>>8;f[1].ethernet[5]=client;
 f[1].ethernet[6]=2;f[1].ethernet[11]=255;f[1].ethernet[12]=8;
}
int main(void)
{
 u8 regs[0x500]={0};u32 base=0x100000;memcpy(regs+0x470,&base,4);
 struct ipa ipa={regs};struct ipa_rtk_controller c={.ipa=&ipa};current=&c;
 c.flows=calloc(4000,sizeof(*c.flows));c.offload_buf=calloc(1,4096+2*8192);
 c.offload_addr=(uintptr_t)c.offload_buf;
 c.header_bank=(struct ipa_rtk_header_bank){&ipa,calloc(1,200*48),0x100000};
 struct ipa_rtk_flow f[2];
 for(unsigned i=0;i<200;i++){make_flow(f,i);assert(!ipa_rtk_flow_install(&c,i,f));}
 assert(publications==200);
 make_flow(f,200);assert(ipa_rtk_flow_install(&c,200,f)==-ENOSPC);
 for(unsigned i=200;i<4000;i++){make_flow(f,i%200);assert(!ipa_rtk_flow_install(&c,i,f));}
 assert(publications==200 && c.total_installs==4000);
 for(unsigned i=0;i<200;i++){assert(c.clients[i].refcount==20);assert(c.headers[i].refcount==1);}
 unsigned order[4000];for(unsigned i=0;i<4000;i++)order[i]=i;
 srand(6543);for(unsigned i=3999;i;i--){unsigned j=rand()%(i+1),t=order[i];order[i]=order[j];order[j]=t;}
 for(unsigned i=0;i<4000;i++)assert(!ipa_rtk_flow_remove(&c,order[i]));
 assert(publications==400 && c.total_removes==4000);
 for(unsigned i=0;i<200;i++){assert(!c.clients[i].refcount);assert(!c.headers[i].refcount);}
 make_flow(f,0);fail_publish=1;
 assert(ipa_rtk_flow_install(&c,0,f)==-EIO);
 assert(c.flows[0].active);
 assert(!ipa_rtk_flow_remove(&c,0));
 assert(!ipa_rtk_flow_install(&c,0,f));fail_publish=1;
 assert(ipa_rtk_flow_remove(&c,0)==-EIO);
 assert(c.flows[0].active && c.clients[c.flows[0].client_index].refcount==1);
 assert(!ipa_rtk_flow_remove(&c,0));
 assert(ipa_header_rtk_bank_set(&c.header_bank,200,f[1].ethernet)==-EINVAL);
 base=0;memcpy(regs+0x470,&base,4);
 assert(ipa_header_rtk_bank_set(&c.header_bank,0,f[1].ethernet)==-ESTALE);
 free(c.header_bank.buffer);free(c.offload_buf);free(c.flows);
 puts("PASS: production route/client/HDR_ADD code; 200 distinct MAC/IP clients, 4000 flows, context offsets, bounded route writes, random teardown and publication failure retention");
}
'''
    (d / 'test.c').write_text(text)
    subprocess.run(['gcc', '-std=gnu11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-I', str(d), str(d / 'test.c'),
                    '-o', str(d / 'test')], check=True)
    subprocess.run([str(d / 'test')], check=True)
