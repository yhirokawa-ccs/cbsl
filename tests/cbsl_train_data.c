/*
 * Copyright 2019 Yuta Hirokawa (University of Tsukuba, Japan)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cbsl.h>

#define CBSL_ERROR_CHECK(X)  {if ((X) == cbsl_error) { fprintf(stderr, "error: %s\n", (#X)); exit(1); }}
#define CHECK(X)             {if (!(X)) { fprintf(stderr, "error: check %s\n", (#X)); exit(1); }}
#define CHECK_BINARY(X,Y)    {if (memcmp(&(X),&(Y),sizeof((X))) != 0) { fprintf(stderr, "error: binary check %s == %s\n", (#X), (#Y)); exit(1); }}

extern void rand_double(uint64_t data_size, double* a);
extern void write_training(uint64_t size, const void* data);
extern void record(cbsl_mode, uint64_t* size, void** data);

char cname[128];

int main(int argc, char** argv)
{
  sprintf(cname, "check_train_data.zst");

  uint64_t size = 16 * 1024 * 1024; /* 16 MiB */
  double*  data = (double*)(malloc(size));

  rand_double(size/sizeof(double), data);
  write_training(size, data);

  uint64_t rsize = 0;
  double*  rdata = NULL;
  record(cbsl_load_mode, &rsize, (void**) &rdata);

  CHECK(size == rsize);
  CHECK(rdata != NULL);
  for (uint64_t i = 0; i < (size/sizeof(double)); ++i)
  {
    CHECK_BINARY(data[i], rdata[i]);
  }

  free(rdata);

  return 0;
}

void write_training(uint64_t size, const void* data)
{
  cbsl_ctx* ctx = cbsl_open(cbsl_store_mode, cname);
  if (ctx == NULL)
  {
    fprintf(stderr, "error: cbsl_open\n");
    exit(1);
  }
  CBSL_ERROR_CHECK(cbsl_train_data(ctx, data, 8, size));
  CBSL_ERROR_CHECK(cbsl_write(ctx, &size, sizeof(size)));
  CBSL_ERROR_CHECK(cbsl_write(ctx, data, size));
  CBSL_ERROR_CHECK(cbsl_close(ctx));
}

void record(cbsl_mode mode, uint64_t* size, void** data)
{
  cbsl_ctx* ctx = cbsl_open(mode, cname);
  if (ctx == NULL)
  {
    fprintf(stderr, "error: cbsl_open\n");
    exit(1);
  }
  CBSL_ERROR_CHECK(cbsl_record_heap(ctx, data, size));
  CBSL_ERROR_CHECK(cbsl_close(ctx));
}

void rand_double(uint64_t data_size, double* a)
{
  for(uint64_t i = 0; i < data_size; ++i)
    a[i] = (double)(rand()) / RAND_MAX;
}
