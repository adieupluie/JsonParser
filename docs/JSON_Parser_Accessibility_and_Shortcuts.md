# JSON Parser Viewer 접근성 및 단축키

## 1. 접근성 원칙

- 모든 상호작용이 키보드만으로 가능해야 함.
- 스크린 리더가 읽을 수 있도록 ARIA 속성 제공.
- 명확한 포커스 표시.
- 충분한 색 대비.

## 2. 키보드 네비게이션

- `Tab` / `Shift+Tab`: 패널 간 이동
- `Arrow Up` / `Arrow Down`: 트리 노드 간 이동
- `Arrow Right`: 노드 확장 / 다음 노드로 이동
- `Arrow Left`: 노드 축소 / 이전 노드로 이동
- `Enter`: 선택 또는 편집 모드 진입
- `Escape`: 편집 취소 또는 모달 닫기

## 3. 단축키

- `Ctrl+O`: 파일 열기
- `Ctrl+S`: 저장
- `Ctrl+P`: 파싱 실행
- `Ctrl+F`: 검색 열기
- `Ctrl+E`: 모두 확장
- `Ctrl+Shift+E`: 모두 축소
- `F8`: 다음 오류로 이동
- `Ctrl+Shift+F`: 노드/값 검색

## 4. 스크린리더 지원

- 트리 노드에 ARIA role="treeitem"
- depth와 child count를 말씀해주는 `aria-level`, `aria-setsize`, `aria-posinset`
- 확장 가능한 노드에 `aria-expanded`
- 선택된 노드에 `aria-selected`

## 5. 색 대비

- 텍스트 대비 최소 4.5:1
- 버튼/상태 태그 대비 최소 3:1
- 오류 메시지 대비 충분히 높게 설정

## 6. 추가 접근성 기능 제안

- 다크 모드/라이트 모드
- 글꼴 크기 조정
- 고대비 모드
- 화면 확대 시 레이아웃 적응
