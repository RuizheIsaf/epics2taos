// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX - License - Identifier: Apache - 2.0

// Enables test_s3-crt-demo.cpp to test the functionality in s3-crt-demo.cpp.

#pragma once

#include <iostream>
#include <fstream>
#include <aws/core/Aws.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/utils/logging/CRTLogSystem.h>
#include <aws/core/utils/UUID.h>
#include <sys/stat.h>

#include <aws/s3/S3Client.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/BucketLocationConstraint.h>
#include <aws/s3/model/DeleteBucketRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/GetObjectResult.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/HeadBucketRequest.h>

#include <stdio.h>
#include <time.h>


#ifdef __cplusplus

extern "C" {
#endif
void aws_initAPI();
void aws_shutdownAPI();
void *s3Client_init();
void s3_upload(void *s3client, void * dbr, char * pvname, size_t dbrsize, unsigned long time);
void s3_upload_asyn(void *s3client, void * dbr, char * pvname, size_t dbrsize, unsigned long time);

bool ListBuckets(const Aws::S3::S3Client& s3Client);
bool CreateBucket(const Aws::S3::S3Client& s3Client, const Aws::String& bucketName, const Aws::S3::Model::BucketLocationConstraint& locConstraint);
bool DeleteBucket(const Aws::S3::S3Client& s3Client, const Aws::String& bucketName);
bool PutObjectFile(const Aws::S3::S3Client& s3Client, const Aws::String& bucketName, const Aws::String& objectKey, const Aws::String& fileName);
bool PutObjectDbr(const Aws::S3::S3Client& s3Client, const Aws::String& bucketName, const Aws::String& objectKey, void *dbr, size_t dbrsize);
void *GetObjectDbr(const Aws::S3::S3Client& s3Client, const Aws::String& bucketName, const Aws::String& objectKey);
bool DeleteObject(const Aws::S3::S3Client& s3Client, const Aws::String& bucketName, const Aws::String& objectKey);

void *getdbr(void *s3client, char *objectKey);
#ifdef __cplusplus
}
#endif



