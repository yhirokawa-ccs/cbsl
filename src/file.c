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
#include <zstd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h> /* memset */

#include "./cbsl_internal.h"

/* FIXME: implicit declaration??? */
extern ZSTDLIB_API size_t ZSTD_DCtx_loadDictionary_byReference(ZSTD_DCtx* dctx, const void* dict, size_t dictSize);

static
cbsl_ctx* cbsl_open_safe_finalize(cbsl_ctx** ctx)
{
  if (*ctx != NULL)
  {
    cbsl_close(*ctx);
  }
  return NULL;
}

cbsl_ctx* cbsl_open(cbsl_mode mode, char* path)
{
  CBSL_CHECK_COND_AND_RETURN(path != NULL, NULL);

  cbsl_ctx* ctx = (cbsl_ctx*)(malloc(sizeof(cbsl_ctx)));
  CBSL_CHECK_COND_AND_RETURN(ctx != NULL, NULL);
  memset(ctx, 0, sizeof(cbsl_ctx));

  char const* fopen_mode;
  switch (mode)
  {
    case cbsl_load_mode:  fopen_mode = "rb"; break;
    case cbsl_store_mode: fopen_mode = "wb"; break;
    default:              free(ctx);         return NULL;
  }
  ctx->mode = mode;
  CBSL_CHECK_COND_AND_RETURN((ctx->fp = fopen(path, fopen_mode)) != NULL, cbsl_open_safe_finalize(&ctx));
  CBSL_CHECK_COND_AND_RETURN(create_streaming_buffers(ctx) == cbsl_success, cbsl_open_safe_finalize(&ctx));

  /* read/write header */
  CBSL_CHECK_COND_AND_RETURN((ctx->dictionary = malloc(MAX_DICTIONARY_SIZE)) != NULL, cbsl_open_safe_finalize(&ctx));
  uint64_t file_version;
  if (mode == cbsl_load_mode)
  {
    file_version = 0;
    CBSL_CHECK_COND_AND_RETURN(fread(&file_version, sizeof(file_version), 1, ctx->fp) == 1, cbsl_open_safe_finalize(&ctx));
    CBSL_CHECK_COND_AND_RETURN(file_version == CBSL_VERSION, cbsl_open_safe_finalize(&ctx));
    CBSL_CHECK_COND_AND_RETURN(fread(&ctx->dict_size_used, sizeof(ctx->dict_size_used), 1, ctx->fp) == 1, cbsl_open_safe_finalize(&ctx));
    CBSL_CHECK_COND_AND_RETURN(fread(ctx->dictionary, 1, MAX_DICTIONARY_SIZE, ctx->fp) == MAX_DICTIONARY_SIZE, cbsl_open_safe_finalize(&ctx));
    if (ctx->dict_size_used > 0)
    {
      const size_t ret = ZSTD_DCtx_loadDictionary_byReference(ctx->zstd_dctx, ctx->dictionary, ctx->dict_size_used);
      CBSL_CHECK_COND_MSG_AND_RETURN(!ZSTD_isError(ret), ZSTD_getErrorName(ret), cbsl_open_safe_finalize(&ctx));
    }
  }
  else if (mode == cbsl_store_mode)
  {
    file_version = CBSL_VERSION;
    CBSL_CHECK_COND_AND_RETURN(fwrite(&file_version, sizeof(file_version), 1, ctx->fp) == 1, cbsl_open_safe_finalize(&ctx));
    CBSL_CHECK_COND_AND_RETURN(fwrite(&ctx->dict_size_used, sizeof(ctx->dict_size_used), 1, ctx->fp) == 1, cbsl_open_safe_finalize(&ctx));
    CBSL_CHECK_COND_AND_RETURN(fwrite(ctx->dictionary, 1, MAX_DICTIONARY_SIZE, ctx->fp) == MAX_DICTIONARY_SIZE, cbsl_open_safe_finalize(&ctx));
  }

  return ctx;
}

cbsl_errors cbsl_close(cbsl_ctx* ctx)
{
  CBSL_CHECK_COND_AND_RETURN(ctx != NULL, cbsl_success);
  if (ctx->mode == cbsl_store_mode)
  {
    cbsl_flush(ctx);
    if (ctx->fp != NULL)
    {
      CBSL_CHECK_COND(fseek(ctx->fp, sizeof(uint64_t), SEEK_SET) == 0);
      CBSL_CHECK_COND(fwrite(&ctx->dict_size_used, sizeof(ctx->dict_size_used), 1, ctx->fp) == 1);
      CBSL_CHECK_COND(fwrite(ctx->dictionary, 1, MAX_DICTIONARY_SIZE, ctx->fp) == MAX_DICTIONARY_SIZE);
      CBSL_CHECK_COND(fseek(ctx->fp, 0, SEEK_END) == 0);
    }
  }
  CBSL_SAFE_FREE_ZSTD_CCTX(ctx->zstd_cctx);
  CBSL_SAFE_FREE_ZSTD_DCTX(ctx->zstd_dctx);
  CBSL_SAFE_FREE(ctx->in_buffer);
  CBSL_SAFE_FREE(ctx->out_buffer);
  CBSL_SAFE_FREE(ctx->dictionary);
  CBSL_SAFE_FCLOSE(ctx->fp);
  CBSL_SAFE_FREE(ctx);
  return cbsl_success;
}
