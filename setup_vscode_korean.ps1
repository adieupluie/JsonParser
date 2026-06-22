# VS Code를 한국어로 설정하는 스크립트
# 사용법: PowerShell에서 이 스크립트를 실행하세요.

$localePath = Join-Path $env:APPDATA 'Code\User\locale.json'
$dir = Split-Path $localePath
if (!(Test-Path $dir)) { New-Item -ItemType Directory -Path $dir -Force | Out-Null }

$localeContent = '{"locale": "ko"}'
[System.Text.Encoding]::UTF8.GetBytes($localeContent) | Set-Content -Encoding Byte -Path $localePath
Write-Output "locale.json을 생성/덮어썼습니다: $localePath"

# VS Code CLI가 설치되어 있으면 한글 언어팩을 설치 시도합니다.
if (Get-Command code -ErrorAction SilentlyContinue) {
    Write-Output "'code' 명령어 발견: 한국어 언어팩 설치를 시도합니다..."
    code --install-extension ms-ceintl.vscode-language-pack-ko
    Write-Output "설치가 완료되면 VS Code를 재시작하세요."
} else {
    Write-Output "'code' 명령어를 찾을 수 없습니다. 수동으로 확장(한국어 언어팩)을 설치하거나 VS Code에서 'Configure Display Language'를 사용하세요."
}
