 빌드
docker-compose -f src/fprime-run.yml down; 
docker-compose -f src/fprime-run.yml up --build;

 내부 터미널 접속
docker exec -it fprime_dev bash; 


cd src/docker; docker-compose up --build libfuzzer;