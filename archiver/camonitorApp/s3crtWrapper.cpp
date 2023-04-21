#include "s3crtWrapper.h"
#include "loadConfig.h"
#include "s3.h"

static const char ALLOCATION_TAG[] = "s3-crt-demo";
Aws::SDKOptions options;

//Turn on logging.


void aws_initAPI()
{
    options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Info;
    // Override the default log level for AWS common runtime libraries to see multipart upload entries in the log file.
    options.loggingOptions.crt_logger_create_fn = []() {
        return Aws::MakeShared<Aws::Utils::Logging::DefaultCRTLogSystem>(ALLOCATION_TAG, Aws::Utils::Logging::LogLevel::Debug);
    };
    //Uncomment the following code to override default global client bootstrap for AWS common runtime libraries.
    // options.ioOptions.clientBootstrap_create_fn = []() {
    //     Aws::Crt::Io::EventLoopGroup eventLoopGroup(0 /* cpuGroup */, 18 /* threadCount */);
    //     Aws::Crt::Io::DefaultHostResolver defaultHostResolver(eventLoopGroup, 8 /* maxHosts */, 300 /* maxTTL */);
    //     auto clientBootstrap = Aws::MakeShared<Aws::Crt::Io::ClientBootstrap>(ALLOCATION_TAG, eventLoopGroup, defaultHostResolver);
    //     clientBootstrap->EnableBlockingShutdown();
    //     return clientBootstrap;
    // };

    // Uncomment the following code to override default global TLS connection options for AWS common runtime libraries.
    // options.ioOptions.tlsConnectionOptions_create_fn = []() {
    //     Aws::Crt::Io::TlsContextOptions tlsCtxOptions = Aws::Crt::Io::TlsContextOptions::InitDefaultClient();
    //     tlsCtxOptions.OverrideDefaultTrustStore(<CaPathString>, <CaCertString>);
    //     Aws::Crt::Io::TlsContext tlsContext(tlsCtxOptions, Aws::Crt::Io::TlsMode::CLIENT);
    //     return Aws::MakeShared<Aws::Crt::Io::TlsConnectionOptions>(ALLOCATION_TAG, tlsContext.NewConnectionOptions());
    // };
    Aws::InitAPI(options);
}

void aws_shutdownAPI() 
{
    
    Aws::ShutdownAPI(options);
}

void * s3crtclient_init(){ 
    Aws::String region = Aws::Region::US_EAST_1;
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
    Aws::S3Crt::ClientConfiguration config;
    config.region = region;
    config.throughputTargetGbps = throughput_target_gbps;
    config.partSize = part_size;
    
    config.endpointOverride = endpoint;
    config.scheme = Aws::Http::Scheme::HTTP;
    config.verifySSL = false;

    Aws::Auth::AWSCredentials cred(ak,sk);
    
    Aws::S3Crt::S3CrtClient *s3_crt_client = new Aws::S3Crt::S3CrtClient(cred, config);
    
    void * s3_crt_client_ptr = s3_crt_client;

    return s3_crt_client_ptr;   
    
   
};

void s3_upload(void *s3_crt_client_ptr, void * dbr) {
    //Aws::InitAPI(options);
    Aws::String file_name = "ny.json";
        
    //TODO: Set to your account AWS Region.
    //Aws::String region = Aws::Region::US_EAST_1;

    //The object_key is the unique identifier for the object in the bucket.
    Aws::String object_key = "my-object";
    
    Aws::String bucket_name = "testbucket";
    
    //Aws::S3Crt::S3CrtClient s3_crt_client = *static_cast<Aws::S3Crt::S3CrtClient *>(s3_crt_client_ptr);
    ListBuckets(*static_cast<Aws::S3Crt::S3CrtClient *>(s3_crt_client_ptr), bucket_name);//注意不能再新建一个S3CrtClient，AWS好像会报错
    //ListBuckets(s3_crt_client, bucket_name);
    //Aws::ShutdownAPI(options);
}