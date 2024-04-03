//==============================================================
// Copyright © 2022 Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================
#include <immintrin.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#define ROW0 16
#define COL0 64
#define ROW1 16
#define COL1 64
#define ROW2 16
#define COL2 64

#define ARCH_GET_XCOMP_PERM     0x1022
#define ARCH_REQ_XCOMP_PERM     0x1023
#define XFEATURE_XTILECFG       17
#define XFEATURE_XTILEDATA      18

//Define tile config data structure 
typedef struct __tile_config
{
  uint8_t palette_id;
  uint8_t start_row;
  uint8_t reserved_0[14];
  uint16_t colsb[16]; 
  uint8_t rows[16]; 
} __tilecfg;

/* Initialize tile config */
static void init_tile_config (__tilecfg *tileinfo)
{

  memset(tileinfo, 0, sizeof(*tileinfo));

  tileinfo->palette_id = 1;
  tileinfo->start_row = 0;

  tileinfo->rows[0] =  ROW0;
  tileinfo->colsb[0] = COL0;

  tileinfo->rows[1] =  ROW1;
  tileinfo->colsb[1] = COL1;

  tileinfo->rows[2] =  ROW2;
  tileinfo->colsb[2] = COL2;

  _tile_loadconfig (tileinfo);
}

/* Initialize int8_t buffer */
static void init_buffer (int8_t *buf, int32_t rows, int32_t colsb, int8_t value)
{
  int i, j;

  for (i = 0; i < rows; i++)
    for (j = 0; j < colsb; j++)
    {
#if 0
        buf[i * colsb + j] = value;
#else
        if (value == 1)
           buf[i * colsb + j] = value;
	else
           buf[i * colsb + j] = i;
#endif
    }
}

/* Initialize int32_t buffer */
static void init_buffer32 (int32_t *buf, int32_t rows, int32_t colsb, int32_t value)
{
  int i, j;
  int colsb2=colsb/4;

  for (i = 0; i < rows; i++)
    for (j = 0; j < (colsb2); j++)
    {
        buf[i * colsb2 + j] = value;
    }
}

/* Set_tiledata_use() - Invoke syscall to set ARCH_SET_STATE_USE */
static bool set_tiledata_use()
{
   if (syscall(SYS_arch_prctl, ARCH_REQ_XCOMP_PERM, XFEATURE_XTILEDATA)) 
   {
      printf("\n Fail to do XFEATURE_XTILEDATA \n\n");
      return false;
   }
   else
   {
      printf("\n TILE DATA USE SET - OK \n\n");
      return true;
   }

   return true;
}

/* Print int8_t buffer */
static void print_buffer(int8_t* buf, int32_t rows, int32_t colsb) 
{
   printf("rows:%d colsb:%d\n", rows, colsb);
   for (int i = 0; i < rows; i++) {
     for (int j = 0; j < (colsb); j++)
     {
         printf("%3d ", buf[i * colsb + j]);
     }
     printf("\n");
   }
   printf("\n");
}

/* Print int32_t buffer */
static void print_buffer32(int32_t* buf, int32_t rows, int32_t colsb)
{
   int colsb2=colsb/4;
   printf("rows:%d colsb2:%d\n", rows, colsb2);
   for (int i = 0; i < rows; i++) {
     for (int j = 0; j < (colsb2); j++)
     {
         printf("%3d ", buf[i * colsb2 + j]);
     }
     printf("\n");
   }
   printf("\n");
}

static   __tilecfg tile_data = {0};
static   int8_t src1[ROW1*COL1];
static   int8_t src2[ROW2*COL2];
static   int32_t res[ROW0*COL0/4];

int main(){


   // Request permission to linux kernel to run AMX 
   if (!set_tiledata_use())
      exit(-1);

   // Load tile configuration 
   init_tile_config (&tile_data);

   // Init src matrix buffers with data
   init_buffer(src1, ROW1, COL1, 1);
   print_buffer(src1, ROW1, COL1);
 
   init_buffer(src2,  ROW2, COL2, 1);
   print_buffer(src2, ROW2, COL2);

   init_buffer32(res, ROW0, COL0, 0);
   print_buffer32 (res, ROW0, COL0);

   // Load tile rows from memory
   _tile_loadd(0, res,  COL0);
   _tile_loadd(1, src1, COL1);
   _tile_loadd(2, src2, COL2);

   // Compute dot-product of bytes in tiles 
   _tile_dpbssd(0, 1, 2);
   _tile_stored(0, res, COL0);
   print_buffer32(res, ROW0, COL0);

   // Release the tile configuration to return to the init state, 
   // which releases all storage it currently holds
   _tile_release ();
}
