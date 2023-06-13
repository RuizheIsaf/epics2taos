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
#include <mutex>

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
#include <mutex>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/Aws.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/utils/threading/Executor.h>


namespace Aws
{
    namespace Client
    {
        /**
        * Call-back context for all async client methods. This allows you to pass a context to your callbacks so that you can identify your requests.
        * It is entirely intended that you override this class in-lieu of using a void* for the user context. The base class just gives you the ability to
        * pass a uuid for your context.
        */
        class AWS_CORE_API ArchiveContext: public AsyncCallerContext
        {
        public:
            /**
             * Initializes object with generated UUID
             */
            ArchiveContext(){};

            /**
             * Sets buffer pointer
             */
            inline void SetBuffPointer(void * p) { pbuff =p; }

            /**
             * free buffer when uploading finished
             */
            inline void FreeBuff() { free(pbuff); }

        private:
            void* pbuff;
        };
    }
}

#ifdef __cplusplus

extern "C" {
#endif
void aws_initAPI();
void aws_shutdownAPI();
void *s3Client_init();
void s3_upload(void *s3client, void * dbr, char * pvname, size_t dbrsize, unsigned long time);
void s3_upload_asyn(void *s3Client, void * dbr, char * pvname, size_t dbrsize, unsigned long time);
bool ListBuckets(const Aws::S3::S3Client& s3Client);
bool CreateBucket(const Aws::S3::S3Client& s3Client, const Aws::String& bucketName);
bool DeleteBucket(const Aws::S3::S3Client& s3Client, const Aws::String& bucketName);
bool PutObjectFile(const Aws::S3::S3Client& s3Client, const Aws::String& bucketName, const Aws::String& objectKey, const Aws::String& fileName);

void PutObjectAsyncFinished(const Aws::S3::S3Client *s3Client,
                            const Aws::S3::Model::PutObjectRequest &request,
                            const Aws::S3::Model::PutObjectOutcome &outcome,
                            const std::shared_ptr<const Aws::Client::AsyncCallerContext> &context);
bool PutObjectDbr(const Aws::S3::S3Client& s3Client, const Aws::String& bucketName, const Aws::String& objectKey, void *dbr, size_t dbrsize);
bool PutObjectDbrAsync(const Aws::S3::S3Client& s3Client, const Aws::String& bucketName, const Aws::String& objectKey, void *dbr, size_t dbrsize);
void *GetObjectDbr(const Aws::S3::S3Client& s3Client, const Aws::String& bucketName, const Aws::String& objectKey);
bool DeleteObject(const Aws::S3::S3Client& s3Client, const Aws::String& bucketName, const Aws::String& objectKey);

void *getdbr(void *s3client, char *objectKey);
#ifdef __cplusplus
}
#endif



