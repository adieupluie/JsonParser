# JSON Parser Viewer 개발자 핸드오프

## 1. 목적

이 문서는 개발 팀이 JSON 파서 뷰어를 구현할 때 참고할 핵심 정보, 데이터 모델, UI 흐름, CLI/EXE 배포 요구사항을 제공합니다.

## 2. 핵심 기능 요약

- JSON 텍스트 입력 및 파일 로드
- JSON 파싱 및 트리 구조 시각화
- 파싱 오류 위치 표시 및 수정 지원
- 선택 노드 상세 정보 확인 및 편집
- GUI/CLI 모두 지원하는 Windows 데스크탑 앱

## 3. 데이터 모델

### 3.1 트리 노드 스키마

노드 객체 예시:

```ts
interface JsonTreeNode {
  id: string;
  key: string;
  type: 'object' | 'array' | 'string' | 'number' | 'boolean' | 'null';
  path: string;
  value?: string | number | boolean | null;
  children?: JsonTreeNode[];
  expanded?: boolean;
  meta?: Record<string, any>;
}
```

### 3.2 파서 결과

- `success: boolean`
- `tree: JsonTreeNode[]`
- `error?: { message: string; line: number; column: number; offset?: number }`
- `durationMs: number`

## 4. UI 컴포넌트 구조

### 4.1 좌측 패널: JSON 에디터

- Multiline text editor
- 라인 넘버, 구문 하이라이트
- `Parse` 버튼, `Parse on Type` 토글
- 에러 하이라이트 및 `Go to Error`

### 4.2 중앙 패널: 트리 뷰

- 객체/배열/값 노드 구분 렌더링
- 확장/축소 토글
- 우클릭 컨텍스트 메뉴
- 검색 결과 하이라이트

### 4.3 우측 패널: 인스펙터

- 선택 노드 정보
- `path`, `type`, `value`
- 편집 후 `Apply`
- `Copy Path`, `Copy Value`

### 4.4 하단: 상태바

- 파싱 상태
- 노드 수
- 선택 경로
- 파싱 시간

## 5. 주요 상호작용 플로우

### 5.1 파싱 플로우

1. 사용자가 JSON 편집기에 텍스트 입력.
2. 입력 후 자동 파싱(500ms debounce) 또는 수동 파싱.
3. 파서가 JSON을 성공적으로 읽으면 트리 모델 생성.
4. 트리 뷰 렌더링 및 상태바 갱신.
5. 실패 시 에디터 하이라이트, 에러 메시지 표시.

### 5.2 노드 편집 동기화

1. 트리에서 노드 선택.
2. 인스펙터에서 값 수정.
3. `Apply` 클릭 시 내부 트리와 에디터 원본 동기화.
4. 수정된 JSON을 다시 파싱하여 일관성 유지.

### 5.3 검색 플로우

- 사용자가 검색어 입력 → 일치 노드 하이라이트 + 결과 목록
- 선택 시 해당 트리 노드로 포커스 이동

## 6. CLI / EXE 배포

### 6.1 EXE 실행 옵션

- `json-tree-viewer.exe`
- `json-tree-viewer.exe -f input.json`
- `json-tree-viewer.exe --validate`
- `json-tree-viewer.exe --export-tree output.json`

### 6.2 GUI/CLI 모드

- GUI: 기본 실행
- CLI: 파일을 파싱하여 결과를 stdout 또는 파일로 출력

### 6.3 Windows 통합

- `.json` 파일 연결
- 탐색기 컨텍스트 메뉴 `Open with JSON Tree Viewer`

## 7. 테스트 및 검증 시나리오

- 유효 JSON 파싱 및 트리 렌더 확인
- 구문 오류 파싱 실패 및 에러 위치 체크
- 배열/객체 중첩 탐색
- 대용량 JSON 성능 검증
- 노드 편집 후 동기화 확인
- 검색/복사/확장/축소 기능

## 8. 제안 구현 스택

- Electron 또는 Tauri 기반 Windows 데스크탑 앱
- React/Vue/Svelte로 UI 구성
- Monaco Editor 또는 CodeMirror 기반 JSON 편집기
- 트리 가상화를 위한 라이브러리
- `jsonc-parser` 또는 표준 JSON 파서
- 배포용 `electron-builder`, `tauri` 또는 `PyInstaller`

## 9. 핸드오프 체크리스트

- [ ] UI 와이어프레임
- [ ] 데이터 모델 정의
- [ ] CLI 옵션 목록
- [ ] 파싱 오류 UX 정의
- [ ] 접근성/단축키 목록
- [ ] 배포 타겟 명세
