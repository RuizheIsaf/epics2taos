#pragma once

#include <iostream>
#include <fstream>
#include <aws/core/Aws.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/utils/logging/CRTLogSystem.h>
#include <aws/s3-crt/S3CrtClient.h>
#include <aws/s3-crt/model/CreateBucketRequest.h>
#include <aws/s3-crt/model/BucketLocationConstraint.h>
#include <aws/s3-crt/model/DeleteBucketRequest.h>
#include <aws/s3-crt/model/PutObjectRequest.h>
#include <aws/s3-crt/model/GetObjectRequest.h>
#include <aws/s3-crt/model/DeleteObjectRequest.h>
#include <aws/core/utils/UUID.h>


#ifdef __cplusplus
extern "C" {
#endif

void aws_initAPI();

void aws_shutdownAPI();

void * s3crtclient_init();

void s3_upload(void *s3_crt_client, void * dbr);

#ifdef __cplusplus
}
#endif