# JSON Parser Viewer 제품 사양서

## 1. 개요

Windows 데스크탑용 JSON 파서/트리 뷰어 앱으로, 텍스트 입력 또는 파일 입력을 받아 JSON을 파싱하고 계층 트리 구조로 시각화합니다. 최종 사용자와 개발자 모두를 위한 기능, 상호작용, 배포 요구사항을 포함합니다.

## 2. 목표

- 사용자 친화적인 JSON 파싱 및 탐색 인터페이스 제공
- 입력 JSON을 트리 구조로 명확하게 표시
- 파싱 오류 위치를 빠르게 찾아 수정 가능
- Windows EXE로 배포 및 CLI 연동 지원

## 3. 주요 화면 구성

### 3.1 메인 화면

- 상단 툴바
  - Open
  - Save
  - Parse
  - Validate
  - Pretty Print
  - Expand All
  - Collapse All
  - Export Tree
- 좌측 편집기 패널
  - Raw JSON 입력
  - 라인 넘버 표시
  - 구문 하이라이트
  - 자동 들여쓰기
  - `Parse on Type` 토글
- 중앙 트리 뷰 패널
  - 계층적 노드 표시
  - 객체 / 배열 / 원시 값 구분 아이콘
  - 노드별 미리보기 값 표시
  - 트리 노드 확장/축소
- 우측 인스펙터 패널
  - 선택 노드 상세 정보
  - 경로, 타입, 값, JSON Pointer
  - 편집 가능 필드
- 하단 상태바
  - 파싱 상태
  - 노드 개수
  - 선택 경로
  - 파싱 시간

## 4. 상호작용 흐름

### 4.1 입력 및 파싱

- 사용자는 직접 편집하거나 JSON 파일을 열 수 있다.
- `Parse on Type` 켜짐 상태에서 입력 중단 후 500ms 지연 후 자동 파싱.
- 수동 `Parse` 버튼 또는 `Ctrl+P`로 즉시 파싱.
- 파싱 성공 시 트리 뷰가 갱신되고 우측 인스펙터는 선택된 노드를 표시.
- 파싱 실패 시 에러 메시지와 위치가 표시되고 이전 유효 상태 트리를 보존.

### 4.2 오류 처리

- 에러 발생 시 편집기 내부에서 라인/컬럼 하이라이트.
- 상태바에 오류 요약 표시.
- 에러 패널 또는 모달에서 상세 메시지 제공.
- `Go to Error` 버튼으로 해당 위치 이동.

### 4.3 트리 상호작용

- 클릭: 노드 선택 및 우측 인스펙터에 로드.
- 확장/축소: 각 객체/배열 노드에 토글 화살표.
- 우클릭 메뉴:
  - Copy Path
  - Copy Value
  - Expand/Collapse
  - Focus in Editor
  - Remove Node
  - Insert Child

### 4.4 노드 편집 동기화

- 인스펙터에서 선택 노드 값 편집 후 `Apply`.
- 내부 트리 모델 업데이트.
- 에디터 원문에 동기화된 JSON 반영.
- 동기화 후 자동 재파싱.

### 4.5 검색 및 필터

- 툴바 또는 별도 검색창에서 텍스트 검색.
- 키워드 일치 노드 하이라이트.
- 결과 목록 제공 및 선택 시 트리에서 포커스.

### 4.6 대용량 JSON 지원

- 트리 가상화로 visible nodes만 렌더.
- 초기 확장 깊이 제한.
- 로딩 스피너 및 처리 시간 표시.
- 파싱 타임아웃 보호.

## 5. CLI / EXE 배포 요구사항

### 5.1 배포 형태

- 기본: `json-tree-viewer.exe` 단일 실행 파일
- 선택: 설치형 MSI 또는 포터블 EXE

### 5.2 실행 모드

- GUI 기본 실행
- CLI 옵션 지원:
  - `json-tree-viewer.exe -f input.json`
  - `json-tree-viewer.exe --export-tree output.json`
  - `json-tree-viewer.exe --validate`

### 5.3 Windows 통합

- Windows 탐색기 우클릭 `Open with JSON Tree Viewer` 옵션
- `.json` 파일과 연결 선택 가능

### 5.4 배포 타겟

- Windows x64
- 나중에 필요 시 x86 또는 ARM64 추가

## 6. 데이터 모델 (트리 구조)

### 6.1 노드 유형

- `object`
- `array`
- `string`
- `number`
- `boolean`
- `null`

### 6.2 노드 구조 예시

```json
{
  "key": "user",
  "type": "object",
  "path": "$.user",
  "children": [
    {
      "key": "name",
      "type": "string",
      "path": "$.user.name",
      "value": "Alice"
    }
  ]
}
```

### 6.3 필드 정의

- `key`: 부모 객체/배열 내 항목 이름 또는 인덱스
- `type`: JSON 타입
- `path`: JSON Pointer 또는 `$` 기반 경로
- `value`: 원시 값(또는 간략 미리보기)
- `children`: 하위 노드 배열
- `expanded`: 화면 렌더 상태
- `meta`: 사용자 주석/추가 정보(옵션)

## 7. 비주얼 디자인 스펙

### 7.1 레이아웃

- 좌측 편집기 40%
- 중앙 트리 뷰 40%
- 우측 인스펙터 20%
- 반응형 비율 조절 가능

### 7.2 색상 팔레트

- 배경: `#FFFFFF`
- 패널 경계: `#E6E6E6`
- 기본 텍스트: `#111111`
- 강조: `#1890FF`
- 오류: `#FF4D4F`
- 노드 hover: `#F5F7FA`

### 7.3 타이포그래피

- 본문: 14px 산세리프
- 제목: 16~18px
- 모노스페이스: JSON 에디터 및 코드 인스펙터

### 7.4 아이콘

- 객체: 폴더 형태
- 배열: 리스트/대괄호
- 문자열: 따옴표
- 숫자: `#`
- boolean/null: 토글/태그

### 7.5 UI 상태

- 성공: 녹색 체크 또는 `Parsed successfully`
- 오류: 빨간 강조 + 상세 텍스트
- 로딩: 스피너 + `Parsing...`

## 8. 접근성 및 키보드 단축키

### 8.1 접근성

- 모든 주요 버튼과 컨트롤에 명확한 라벨
- 키보드 포커스 스타일
- 스크린리더용 ARIA 역할 및 상태
- 충분한 컬러 대비
- `Tab` 순서 최적화

### 8.2 권장 단축키

- `Ctrl+O`: Open
- `Ctrl+S`: Save
- `Ctrl+P`: Parse
- `Ctrl+F`: Search
- `Ctrl+E`: Expand All
- `Ctrl+Shift+E`: Collapse All
- `F8`: 다음 오류로 이동
- `Enter`: 트리 노드 선택/편집
- `Delete`: 노드 삭제

## 9. 개발자 핸드오프

### 9.1 핵심 API 계약

- 입력: JSON 문자열 또는 파일 경로
- 출력: 트리 노드 배열
- 에러: 라인, 컬럼, 메시지
- 노드 선택: `path` 기반 탐색
- 편집 적용: 노드 변경 후 원문 동기화

### 9.2 테스트 시나리오

- 유효 JSON 파싱 성공
- 구문 오류 파싱 실패
- 중첩 객체/배열 탐색
- 대용량 JSON 성능
- 검색/필터/복사 기능
- 노드 편집 후 동기화

### 9.3 제출 자료

- 와이어프레임 설명
- JSON 트리 데이터 모델 스펙
- CLI 옵션 정의
- 오류 처리 플로우
- 접근성 체크리스트

## 10. 추가 제안

- 다크 모드 테마
- JSON Schema 기반 유효성 검사 확장
- JSON Pointer/JSONPath 복사 기능
- 트리 내 노드 필터링 및 북마크
