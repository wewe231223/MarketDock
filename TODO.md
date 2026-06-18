# TODO

## 1. 개발 환경

- [ ] Visual Studio에서 `x64 Debug`, `x64 Release` 빌드 확인
- [ ] `vcpkg integrate install` 기준 의존성 복원 확인
- [ ] `vcpkg_installed` 산출물이 Git에 포함되지 않는지 확인
- [ ] 프로젝트 기본 실행 파일 이름, 아이콘, 리소스 정리

## 2. 프로젝트 구조 정리

### 솔루션 프로젝트 설명

- `MarketDock`: 최종 사용자에게 제공되는 Win32 데스크톱 실행 프로젝트이다. Win32 진입점, 창 생명주기, DirectX12 렌더링, ImGui/ImPlot UI, 오버레이 창 기능을 담당한다. `Core`와 `Network`를 사용하지만, 핵심 데이터 모델이나 Finnhub 통신 세부 구현을 직접 소유하지 않는다.
- `Core`: 플랫폼 의존성을 최소화한 순수 로직 정적 라이브러리이다. 설정 데이터, 시장 데이터 모델, `QuoteCache`, 상태 계산 규칙, 공통 타입을 담당한다. Win32, DirectX12, ImGui, 네트워크 구현에 의존하지 않는 것을 원칙으로 한다.
- `Network`: Finnhub REST와 WebSocket 통신을 담당하는 정적 라이브러리이다. HTTPS 요청, WSS 연결, JSON 파싱, 재연결, 구독 관리, 네트워크 스레드 작업을 담당한다. 시장 데이터 표현과 캐시 갱신 계약은 `Core`의 타입을 사용한다.
- `Test`: 데이터와 네트워크 로직을 검증하기 위한 콘솔 실행 프로젝트이다. UI나 렌더링 없이 `Core`와 `Network`의 파싱, 계산, 캐시 동작, 통신 흐름을 빠르게 확인하는 용도로 사용한다.

- [ ] `Application` 계층 생성
- [ ] `Configuration` 계층 생성
- [ ] `Data` 계층 생성
- [ ] `Network` 계층 생성
- [ ] `UI` 계층 생성
- [ ] Win32 진입점에서 애플리케이션 생명주기 클래스로 책임 분리
- [ ] 공통 타입과 유틸리티 위치 결정

## 3. Win32 창 기능

- [ ] 기본 Win32 창 생성 코드 정리
- [ ] Borderless Window 적용
- [ ] Always On Top 적용
- [ ] Transparent Background 적용
- [ ] Click Through Mode 적용
- [ ] Compact Mode 적용
- [ ] DPI Awareness 설정
- [ ] 창 위치와 크기 저장 및 복원

## 4. DirectX12 렌더링

- [ ] DirectX12 Device 초기화
- [ ] Command Queue 생성
- [ ] Swap Chain 생성
- [ ] Descriptor Heap 생성
- [ ] Render Target View 생성
- [ ] Frame Context 관리
- [ ] Resize 처리
- [ ] Device Lost 처리
- [ ] ImGui Win32 백엔드 연결
- [ ] ImGui DirectX12 백엔드 연결
- [ ] ImPlot Context 생성 및 해제

## 5. 설정 파일

- [ ] `config.json` 기본 스키마 정의
- [ ] Finnhub API Key 로드
- [ ] 종목 목록 로드
- [ ] 창 설정 로드
- [ ] UI 설정 로드
- [ ] 설정 파일이 없을 때 기본 파일 생성
- [ ] 잘못된 설정 값 검증 및 fallback 처리

## 6. 핵심 데이터 모델

- [ ] `QuoteData` 정의
- [ ] `ConnectionState` 정의
- [ ] 가격, 등락폭, 등락률 계산 규칙 정의
- [ ] timestamp 단위 통일
- [ ] 유효하지 않은 값 표현 방식 결정

## 7. QuoteCache

- [ ] `QuoteCache` 클래스 구현
- [ ] `std::shared_mutex` 기반 읽기/쓰기 보호
- [ ] 종목별 snapshot 저장
- [ ] WebSocket trade 반영
- [ ] REST quote 보정 반영
- [ ] UI용 snapshot 복사 API 제공
- [ ] STALE 판정 기준 구현
- [ ] 종목 추가 및 삭제 API 구현

## 8. Finnhub REST

- [ ] HTTPS REST 클라이언트 구현
- [ ] Quote API 요청 구현
- [ ] REST 응답 JSON 파싱
- [ ] HTTP 오류 처리
- [ ] Finnhub 오류 응답 처리
- [ ] API Key 누락 처리
- [ ] 초기 snapshot 로드 흐름 구현
- [ ] 5분 주기 보정 작업 구현

## 9. Finnhub WebSocket

- [ ] WSS 클라이언트 구현
- [ ] 연결 및 TLS handshake 구현
- [ ] 종목 subscribe 메시지 전송
- [ ] trade 메시지 수신
- [ ] WebSocket JSON 파싱
- [ ] ping 또는 keepalive 처리
- [ ] 연결 종료 감지
- [ ] 재연결 backoff 구현
- [ ] 재연결 후 기존 종목 재구독
- [ ] 네트워크 스레드 종료 처리

## 10. 스레드 구조

- [ ] Main Thread와 Network Thread 책임 분리
- [ ] 네트워크 작업 큐 설계
- [ ] UI에서 종목 추가 및 삭제 요청 전달
- [ ] 종료 시 네트워크 스레드 join 보장
- [ ] 예외 발생 시 애플리케이션 종료 흐름 정리

## 11. UI

- [ ] 기본 위젯 레이아웃 구현
- [ ] 심볼 표시
- [ ] 현재가 표시
- [ ] 등락폭 표시
- [ ] 등락률 표시
- [ ] 고가, 저가, 시가 표시
- [ ] LIVE, STALE, ERROR 상태 표시
- [ ] Compact Mode 레이아웃 구현
- [ ] 설정창 구현
- [ ] 종목 추가 및 삭제 UI 구현
- [ ] API Key 입력 UI 구현
- [ ] 색상 규칙 정의
- [ ] 폰트 설정

## 12. 차트 확장 준비

- [ ] 종목별 최근 trade history 저장 구조 설계
- [ ] history 최대 길이 제한
- [ ] ImPlot 기반 미니 차트 prototype 구현
- [ ] 차트 표시 on/off 설정
- [ ] 차트 렌더링 비용 측정

## 13. 로깅

- [ ] spdlog 초기화
- [ ] 로그 파일 위치 결정
- [ ] 네트워크 오류 로그 기록
- [ ] 재연결 로그 기록
- [ ] 설정 로드 오류 로그 기록
- [ ] Release 빌드 로그 레벨 결정

## 14. 오류 처리

- [ ] API Key 없음
- [ ] config 파싱 실패
- [ ] REST 요청 실패
- [ ] WebSocket 연결 실패
- [ ] JSON 파싱 실패
- [ ] 종목별 데이터 없음
- [ ] DirectX12 초기화 실패
- [ ] 사용자에게 표시할 오류 상태 정의

## 15. 배포 준비

- [ ] Release 빌드 산출물 정리
- [ ] 필요한 DLL 배포 확인
- [ ] 기본 `config.json` 샘플 작성
- [ ] README 작성
- [ ] 라이선스 확인
- [ ] 실행 파일 버전 정보 정리

## 16. MVP 완료 기준

- [ ] `config.json`에서 종목과 API Key를 읽는다
- [ ] 시작 시 REST Quote API로 snapshot을 채운다
- [ ] WebSocket Trade Stream으로 현재가를 갱신한다
- [ ] QuoteCache가 모든 시장 데이터의 단일 접근 지점이다
- [ ] UI는 QuoteCache snapshot만 읽는다
- [ ] Always On Top, Borderless, Transparent가 동작한다
- [ ] 연결 실패 후 자동 재연결한다
- [ ] 일반 사용에서 눈에 띄는 끊김 없이 동작한다

## 17. 후순위 성능 점검

- [ ] MVP 구현 후 평균 CPU 사용량 확인
- [ ] MVP 구현 후 메모리 사용량 확인
- [ ] 불필요한 렌더링이 눈에 띄면 프레임 제한 정책 결정
- [ ] 20개 종목 기준 사용성이 불편하지 않은지 확인
- [ ] WebSocket 메시지 폭주 상황은 필요할 때 별도로 테스트
