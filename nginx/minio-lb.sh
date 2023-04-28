docker run \
    -e "EXECUTOR_CORES=1" \
    -e "EXECUTOR_INSTANCES=4" \
    -e "EXECUTOR_MEMORY=512m" \
    -e "EXECUTOR_LABELS=version=2.4.5" \
    --name minio-lb \
    --network host \
    -p 19000:19000 \
    docker.io/minio/sidekick:v2.0.0 \
    --health-path /minio/health/ready \
    --address :19000 \
    HTTP://172.16.0.171:9000 \
    HTTP://172.16.0.172:9000 \
    HTTP://172.16.0.173:9000

~

