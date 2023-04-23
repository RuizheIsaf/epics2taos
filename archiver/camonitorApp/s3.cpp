#include "s3.h"
#include "loadConfig.h"
#include <unistd.h>


static const char ALLOCATION_TAG[] = "camonitor";
Aws::SDKOptions options;
Aws::String region;
std::mutex upload_mutex;
std::condition_variable upload_variable;


// List all Amazon Simple Storage Service (Amazon S3) buckets under the account.
bool ListBuckets(const Aws::S3::S3Client& s3Client) {

    Aws::S3::Model::ListBucketsOutcome outcome = s3Client.ListBuckets();

    if (outcome.IsSuccess()) {
        std::cout << "All buckets under my account:" << std::endl;

        for (auto const& bucket : outcome.GetResult().GetBuckets())
        {
            std::cout << "  * " << bucket.GetName() << std::endl;
        }
        std::cout << std::endl;

        return true;
    }
    else {
        std::cout << "ListBuckets error:\n"<< outcome.GetError() << std::endl << std::endl;

        return false;
    }
}

// Create an Amazon Simple Storage Service (Amazon S3) bucket.
bool CreateBucket(const Aws::S3::S3Client& s3Client, const Aws::String& bucketName, const Aws::S3::Model::BucketLocationConstraint& locConstraint) {

    std::cout << "Creating bucket: \"" << bucketName << "\" ..." << std::endl;

    Aws::S3::Model::CreateBucketRequest request;
    request.SetBucket(bucketName);

    //  If you don't specify an AWS Region, the bucket is created in the US East (N. Virginia) Region (us-east-1)
    if (locConstraint != Aws::S3::Model::BucketLocationConstraint::us_east_1)
    {
        Aws::S3::Model::CreateBucketConfiguration bucket_config;
        bucket_config.SetLocationConstraint(locConstraint);

        request.SetCreateBucketConfiguration(bucket_config);
    }

    Aws::S3::Model::CreateBucketOutcome outcome = s3Client.CreateBucket(request);

    if (outcome.IsSuccess()) {
        std::cout << "Bucket created." << std::endl << std::endl;

        return true;
    }
    else {
        std::cout << "CreateBucket error:\n" << outcome.GetError() << std::endl << std::endl;

        return false;
    }
}

// Delete an existing Amazon S3 bucket.
bool DeleteBucket(const Aws::S3::S3Client& s3Client, const Aws::String& bucketName) {

    std::cout << "Deleting bucket: \"" << bucketName << "\" ..." << std::endl;

    Aws::S3::Model::DeleteBucketRequest request;
    request.SetBucket(bucketName);

    Aws::S3::Model::DeleteBucketOutcome outcome = s3Client.DeleteBucket(request);

    if (outcome.IsSuccess()) {
        std::cout << "Bucket deleted." << std::endl << std::endl;

        return true;
    }
    else {
        std::cout << "DeleteBucket error:\n" << outcome.GetError() << std::endl << std::endl;

        return false;
    }
}

// Put an Amazon S3 object to the bucket.
bool PutObjectFile(const Aws::S3::S3Client& s3Client, const Aws::String& bucketName, const Aws::String& objectKey, const Aws::String& fileName) {

    std::cout << "Putting object: \"" << objectKey << "\" to bucket: \"" << bucketName << "\" ..." << std::endl;

    Aws::S3::Model::PutObjectRequest request;
    request.SetBucket(bucketName);
    request.SetKey(objectKey);
    std::shared_ptr<Aws::IOStream> bodyStream = Aws::MakeShared<Aws::FStream>(ALLOCATION_TAG, fileName.c_str(), std::ios_base::in | std::ios_base::binary);
    if (!bodyStream->good()) {
        std::cout << "Failed to open file: \"" << fileName << "\"." << std::endl << std::endl;
        return false;
    }
    request.SetBody(bodyStream);
    
    //A PUT operation turns into a multipart upload using the s3-crt client.
    //https://github.com/aws/aws-sdk-cpp/wiki/Improving-S3-Throughput-with-AWS-SDK-for-CPP-v1.9
    Aws::S3::Model::PutObjectOutcome outcome = s3Client.PutObject(request);
    //auto outcome = s3CrtClient.PutObject(request);
    if (outcome.IsSuccess()) {
        std::cout << "Object added." << std::endl << std::endl;

        return true;
    }
    else {
        std::cout << "PutObject error:\n" << outcome.GetError() << std::endl << std::endl;
        return false;
    }
}

bool PutObjectDbr(const Aws::S3::S3Client& s3Client, const Aws::String& bucketName, const Aws::String& objectKey, void *dbr, size_t dbrsize){
    std::cout << "Putting object: \"" << objectKey << "\" to bucket: \"" << bucketName << "\" ..." << std::endl;

    Aws::S3::Model::PutObjectRequest request;
    request.WithBucket(bucketName).WithKey(objectKey);

    //std::stringstream data_stream;
    //data_stream.write(reinterpret_cast<char*>(dbr), dbrsize);
    
    auto data = Aws::MakeShared<Aws::StringStream>("PutObjectInputStream", std::stringstream::in | std::stringstream::out | std::stringstream::binary);
    
    data->write(static_cast<char*>(dbr), dbrsize);
    

    request.SetBody(data);


    auto outcome = s3Client.PutObject(request);
    if (outcome.IsSuccess()) {
        std::cout << "Object added." << std::endl << std::endl;
        return true;
    }
    else {
        std::cout << "PutObject error:\n" << outcome.GetError() << std::endl << std::endl;
        return false;
    }

}

bool PutObjectDbrAsync(const Aws::S3::S3Client& s3Client, const Aws::String& bucketName, const Aws::String& objectKey, void *dbr, size_t dbrsize){
    std::cout << "Putting object: \"" << objectKey << "\" to bucket: \"" << bucketName << "\" ..." << std::endl;

    Aws::S3::Model::PutObjectRequest request;
    request.WithBucket(bucketName).WithKey(objectKey);

    //std::stringstream data_stream;
    //data_stream.write(reinterpret_cast<char*>(dbr), dbrsize);
    
    auto data = Aws::MakeShared<Aws::StringStream>("PutObjectInputStream", std::stringstream::in | std::stringstream::out | std::stringstream::binary);
    
    data->write(static_cast<char*>(dbr), dbrsize);
    

    request.SetBody(data);

    std::shared_ptr<Aws::Client::AsyncCallerContext> context =
            Aws::MakeShared<Aws::Client::AsyncCallerContext>("PutObjectAllocationTag");
    context->SetUUID(objectKey);

    s3Client.PutObjectAsync(request, PutObjectAsyncFinished, context);

    return true;


    // auto outcome = s3Client.PutObjectAsync(request, [](const Aws::S3::S3Client*, const Aws::S3::Model::PutObjectRequest&, const Aws::S3::Model::PutObjectOutcome&, const std::shared_ptr<const Aws::Client::AsyncCallerContext>&) {
    //         if (outcome.IsSuccess()) {
    //             std::cout << "Object uploaded successfully." << std::endl;
    //         } else {
    //             std::cout << "Error uploading object: " << outcome.GetError().GetMessage() << std::endl;
    //         }
    //     });

    // //auto outcome = s3Client.PutObject(request);
    // if (outcome.IsSuccess()) {
    //     std::cout << "Object added." << std::endl << std::endl;
    //     return true;
    // }
    // else {
    //     std::cout << "PutObject error:\n" << outcome.GetError() << std::endl << std::endl;
    //     return false;
    // }

}

// Get the Amazon S3 object from the bucket.
void* GetObjectDbr(const Aws::S3::S3Client& s3Client, const Aws::String& bucketName, const Aws::String& objectKey) {

    std::cout << "Getting object: \"" << objectKey << "\" from bucket: \"" << bucketName << "\" ..." << std::endl;

    Aws::S3::Model::GetObjectRequest request;
    
    request.SetBucket(bucketName);
    request.SetKey(objectKey);

    Aws::S3::Model::GetObjectOutcome outcome = s3Client.GetObject(request);

    if (outcome.IsSuccess()) {
       //Uncomment this line if you wish to have the contents of the file displayed. Not recommended for large files
       // because it takes a while.
       // std::cout << "Object content: " << outcome.GetResult().GetBody().rdbuf() << std::endl << std::endl;
        auto& object = outcome.GetResultWithOwnership().GetBody();
        std::streambuf* buf = object.rdbuf();
        //Aws::OFStream local_file("local-file.txt", std::ios::out | std::ios::binary);
        //local_file << object.rdbuf();
        std::string filename = "local-file.txt"; 
        std::ifstream file(filename, std::ios::binary);
        if (file.is_open()) {
            char buffer[1024];
            while (file.read(buffer, sizeof(buffer)))
            {
                // 处理读取到的数据
            }
            if (file.gcount() > 0)
            {
                // 处理最后剩余的数据
            }
             std::cout << "buffer:"<< buffer << std::endl;
        
        } else {
            std::cout << "Unable to open file" << std::endl;
        }
        
        file.close();
       
        
        int bufsize = 96;
        char cbuf[96] = {0};
        std::streamsize count  = buf->sgetn(cbuf, bufsize);
         /*
        auto& object_stream = outcome.GetResultWithOwnership().GetBody();
        auto object_size = object_stream.tellg();
        object_stream.seekg(0, std::ios::beg);
        void* object_data = new char[object_size];
        object_stream.read((char*)object_data, object_size);
        delete[] (char*)object_data;

        //std::streamsize size = 96;
        //sstream.read(result, size);
        return object_data;
        */
        return 0;
        

        
    }
    else {
        std::cout << "GetObject error:\n" << outcome.GetError() << std::endl << std::endl;
        return 0;
    }

    //auto data = Aws::MakeShared<Aws::StringStream>("PutObjectInputStream", std::stringstream::in | std::stringstream::out | std::stringstream::binary);
    // auto& objectStream = outcome.GetResult().GetBody();
    // auto data = objectStream.rdbuf()
    // std::streambuf* buffer = objectStream.rdbuf();
    // size_t size = buffer->in_avail();
    // char * buffer_data = new char[size];
    // buffer->sgetn(buffer_data, size);

    // void* object_data = static_cast<void*>(buffer_data);
    // return object_data;
    

}

// Delete the Amazon S3 object from the bucket.
bool DeleteObject(const Aws::S3::S3Client& s3Client, const Aws::String& bucketName, const Aws::String& objectKey) {

    std::cout << "Deleting object: \"" << objectKey << "\" from bucket: \"" << bucketName << "\" ..." << std::endl;

    Aws::S3::Model::DeleteObjectRequest request;
    request.SetBucket(bucketName);
    request.SetKey(objectKey);

    Aws::S3::Model::DeleteObjectOutcome outcome = s3Client.DeleteObject(request);

    if (outcome.IsSuccess()) {
        std::cout << "Object deleted." << std::endl << std::endl;

        return true;
    }
    else {
        std::cout << "DeleteObject error:\n" << outcome.GetError() << std::endl << std::endl;

        return false;
    }
}

void aws_initAPI()
{
    Aws::InitAPI(options);
}

void aws_shutdownAPI() 
{  
    Aws::ShutdownAPI(options);
}

void * s3Client_init(){ 
    region = Aws::Region::US_EAST_1;
    const double throughput_target_gbps = 5;
    const uint64_t part_size = 8 * 1024 * 1024; // 8 MB.

    char  **fileData = NULL;
    int lines = 0;
    struct ConfigInfo* info = NULL;
    loadFile_configFile("./config.ini", &fileData, &lines);
    parseFile_configFile(fileData, lines, &info);
    char *endpoint = getInfo_configFile("s3_endpoint", info, lines);
    char *ak = getInfo_configFile("s3_accesskey", info, lines);
    char *sk = getInfo_configFile("s3_secretkey", info, lines);

    Aws::Client::ClientConfiguration cfg;
    cfg.endpointOverride = endpoint;
    cfg.scheme = Aws::Http::Scheme::HTTP;
    cfg.verifySSL = false;

    Aws::Auth::AWSCredentials cred(ak,sk);
    
    Aws::S3::S3Client *s3Client = new Aws::S3::S3Client(cred, cfg, Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never, true);
    
    void *s3Client_ptr = s3Client;
    return s3Client_ptr; 
   
}

void s3_upload(void *s3Client, void * dbr, char * pvname, size_t dbrsize, unsigned long time) {

    Aws::S3::Model::PutObjectRequest request;

    Aws::S3::Model::BucketLocationConstraint locConstraint = Aws::S3::Model::BucketLocationConstraintMapper::GetBucketLocationConstraintForName(region);

    Aws::String bucket_name = "pvarray-bucket";

    Aws::S3::Model::HeadBucketRequest hbr;
    hbr.SetBucket(bucket_name);

    //如果bucket不存在，先创建bucket
    if(!(*static_cast<Aws::S3::S3Client *>(s3Client)).HeadBucket(hbr).IsSuccess()){
        CreateBucket(*static_cast<Aws::S3::S3Client *>(s3Client), bucket_name, locConstraint);
    }    

    Aws::String object_key;
    object_key = pvname + std::to_string(time);
    //std::cout << object_key << std::endl << std::endl;
    //Aws::String bucket_name = "my-bucket";
    std::unique_lock<std::mutex> lock(upload_mutex);
    PutObjectDbrAsync(*static_cast<Aws::S3::S3Client *>(s3Client), bucket_name, object_key, dbr, dbrsize);
    std::cout << "main: Waiting for file upload attempt..." << std::endl << std::endl;
    upload_variable.wait(lock);
    std::cout << std::endl << "main: File upload attempt completed." << std::endl;
    //bool outcome = PutObjectDbr(*static_cast<Aws::S3::S3Client *>(s3Client), bucket_name, object_key, dbr, dbrsize);
}

void *getdbr(void *s3Client, char *objectKey){
    Aws::String object_key = "zheng1:compressExample1679650152938633705";
    Aws::String bucket_name = "pvarray-bucket";

    return GetObjectDbr(*static_cast<Aws::S3::S3Client *>(s3Client), bucket_name, object_key);
}

void PutObjectAsyncFinished(const Aws::S3::S3Client *s3Client,
                            const Aws::S3::Model::PutObjectRequest &request,
                            const Aws::S3::Model::PutObjectOutcome &outcome,
                            const std::shared_ptr<const Aws::Client::AsyncCallerContext> &context) {
    if (outcome.IsSuccess()) {
        std::cout << "Success: PutObjectAsyncFinished: Finished uploading '"
                  << context->GetUUID() << "'." << std::endl;
    }
    else {
        std::cerr << "Error: PutObjectAsyncFinished: " <<
                  outcome.GetError().GetMessage() << std::endl;
    }

    // Unblock the thread that is waiting for this function to complete.
    upload_variable.notify_one();
}
