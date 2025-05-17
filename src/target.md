
* [Introducing F Prime](https://fprime.jpl.nasa.gov/overview/)
* [NASA fprime github](https://github.com/nasa/fprime)


5월 2주차: fprime 및 fuzzer 설치 (docker-compose 기반으로 동일환 환경 구현)
5월 3주차: fprime에 fuzzer 적용 방법 공부
5월 4주차: fuzzer 적용 시행착오
5월 5주차: 결과 정리 및 공유

 빌드
docker-compose -f src/fprime-run.yml down; 
docker-compose -f src/fprime-run.yml up --build;

 내부 터미널 접속
docker exec -it fprime_dev bash; 