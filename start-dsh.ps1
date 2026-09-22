param(
  [string]$Repository = '',
  [string]$Proxy = 'http://127.0.0.1:7897'
)

if (-not $Repository) {
  $Repository = Join-Path $PSScriptRoot 'deepseek-harness'
}
if (-not (Test-Path -LiteralPath (Join-Path $Repository 'package.json'))) {
  throw "Missing local DSH gateway project: $Repository"
}

$secretFile = Join-Path $PSScriptRoot 'secrets.env'
if (-not (Test-Path -LiteralPath $secretFile)) {
  throw "Missing private secret file: $secretFile"
}

Get-Content -LiteralPath $secretFile | ForEach-Object {
  $line = $_.Trim()
  if ($line -and -not $line.StartsWith('#')) {
    $pair = $line.Split('=', 2)
    if ($pair.Count -eq 2 -and $pair[0].Trim()) {
      Set-Item -Path ("Env:" + $pair[0].Trim()) -Value $pair[1].Trim()
    }
  }
}

if (-not $env:HTTPS_PROXY) { $env:HTTPS_PROXY = $Proxy }
if (-not $env:HTTP_PROXY) { $env:HTTP_PROXY = $Proxy }
if (-not $env:ALL_PROXY) { $env:ALL_PROXY = $Proxy }
Push-Location $Repository
try { pnpm dsh web --trusted-host jadeview } finally { Pop-Location }
