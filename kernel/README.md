# kernel
- gpio : gpio 제어 예제.
    - gpio_kernelarea  : 커널 영역 gpio 제어
        - module
            - gpio_module 
                - raspberry 5 의 gpio 메모리 영역에 직접 접근
                - 어플리케이션으로부터 얻은 데이터 처리 (gpio.c)
                    - 1: 켜짐 , 0: 꺼짐
            - gpiofunction_module
                - gpio 커널 내장 라이브러리 활용 (<linux/gpio.h>)
                - 어플리케이션으로부터 얻은 데이터 처리 (gpio.c)
                    - 1: 켜짐 , 0: 꺼짐
            - gpioirq_module
                - 스위치 인터럽트 활용
                - 어플리케이션 없어도 동작. 
            - gpiotimer_module
                - 커널 제공 타이머 활용
                - 스위치 인터럽트도 활용하기 위해 mutex 도입.
                - 어플리케이션으로부터 얻은 데이터 처리 (gpio.c)
                    - 1: 타이머 켜짐, 0: 타이머 꺼짐
            - gpiosignal_module
                - 어플리케이션으로의 시그널 전달 (catch_sign짐l.c)
                    - 스위치 입력 시 어플리케이션 종료.
        
    - gpio_userarea : 유저 영역 gpio 제어

- module : 기본 모듈 예제.



## 주의사항

### 커널 모듈 실습 시 환경 설정
- module 빌드 후 insmod 
- sudo mknod /dev/<device_name> <device type> <주번호> <부번호> 명령으로 디바이스 파일 추가
- sudo chmod 666 /dev/<device_name> 으로 권한 변경.
