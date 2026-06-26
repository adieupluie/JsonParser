# JSON Parser Viewer 개선안 및 바이브 코딩 전략

## 1. 편의성 증가

- 붙여넣기 후 자동 정리: 한 줄 JSON을 붙여넣으면 자동으로 pretty print하거나, 사용자에게 `Pretty Print` 실행을 제안한다.
- Parse on Type: 입력 후 일정 시간 입력이 멈추면 자동 검증하고 상태바에 결과를 표시한다.
- 오류 이동 강화: `F8`로 다음 오류 위치로 이동하고, 오류 라인에 배경 강조를 표시한다.
- 검색 경험 개선: `Ctrl+F` 검색창에서 Enter로 다음 결과, Shift+Enter로 이전 결과, 결과 개수를 상태바에 표시한다.
- 트리 탐색 보조: 선택 노드의 JSON path를 상태바에 표시하고, 우클릭 메뉴로 Copy Path / Copy Value를 제공한다.
- 최근 파일: File 메뉴에 최근 열었던 JSON 파일 목록을 제공한다.
- 상태 유지: splitter 위치, 창 크기, 마지막 폴더, 검색어를 저장해 다음 실행 시 복원한다.

## 2. 가독성 증가

- JSON pretty print: 붙여넣기나 열기 직후 `Ctrl+Shift+P` 또는 버튼으로 들여쓰기 정리.
- 라인 번호: 편집창 왼쪽에 라인 번호 영역을 제공한다.
- 오류 라인 강조: 파싱 실패 시 라인과 컬럼을 계산해 해당 위치를 선택하고 강조한다.
- 색상 구문 강조: 문자열, 숫자, boolean/null, 괄호를 다른 색으로 표시한다.
- 긴 값 축약: 트리 노드에서는 긴 문자열을 짧게 보여주고, 전체 값은 inspector나 tooltip에서 보여준다.
- 폰트 설정: JSON 편집용 monospace 폰트 크기 조절 기능을 제공한다.
- 정규화된 줄바꿈: 파일 열기/붙여넣기 시 CRLF/LF를 내부 기준으로 정리해 라인 이동 정확도를 높인다.

## 3. 단축키 구현 현황

- 구현됨: `Ctrl+O`, `Ctrl+S`, `Ctrl+P`, `Ctrl+A`, `Ctrl+F`, `Ctrl+Shift+F`, `Ctrl+E`, `Ctrl+Shift+E`, `F8`.
- 다음 후보: `Shift+Enter` 검색 이전 결과, `Ctrl+Shift+P` pretty print, `Ctrl+L` 선택 노드 path 복사.

## 4. 소스 구조 개선 방향

- `JSONTreeViewer.cpp`: Win32 창 생성, 메시지 처리, 메뉴/단축키, 레이아웃.
- `JsonParser.*`: JSON 파싱과 오류 위치 계산.
- `JsonModel.h`: 파서와 트리가 공유하는 데이터 모델.
- `TreeViewUtil.*`: 트리 노드 추가, 전체 확장/축소, 트리 검색.
- `FileUtil.*`: UTF-8/UTF-16 파일 읽기와 저장.
- 다음 단계: 검색 UI, pretty printer, app settings를 각각 별도 파일로 분리한다.

## 5. 바이브 코딩 전략

- 한 번에 하나의 사용자 흐름만 바꾼다. 예: “검색창 표시 -> 검색 실행 -> 검색 결과 이동”을 한 PR/커밋으로 묶는다.
- 매 작업 후 Release x64 빌드를 실행한다. 이 프로젝트는 리소스/SDK/toolset 문제도 빌드에서 바로 드러난다.
- 기능 요청은 docs의 UX 문장과 코드의 단축키/메뉴 ID를 같이 갱신한다.
- 큰 리팩터링 전에는 먼저 파일 경계를 만든다. 이후 동작 변경은 작은 함수 단위로 이동한다.
- Win32 메시지 처리는 `HandleCommand`, `LayoutChildren`, `WindowProc`처럼 역할별 함수로 유지한다.
- 사용자가 체감하는 변경은 상태바 문구로 남긴다. 작은 기능도 “됐는지” 보이는 피드백이 중요하다.
- 자동 테스트가 부족하므로 파서 로직은 다음 단계에서 콘솔 테스트 또는 단위 테스트 프로젝트로 분리한다.
