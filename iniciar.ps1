$ErrorActionPreference = 'Stop'
$previous = $env:SUPABASE_PASSWORD
try {
    if (!$env:SUPABASE_PASSWORD) {
        $secret = Read-Host 'Digite sua senha' -AsSecureString
        $pointer = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($secret)
        try { $env:SUPABASE_PASSWORD = [Runtime.InteropServices.Marshal]::PtrToStringBSTR($pointer) }
        finally { [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($pointer) }
    }
    & (Join-Path $PSScriptRoot 'dist/DoceHamburgueria-Windows.exe')
    if ($LASTEXITCODE -ne 0) { throw 'O programa encerrou com erro. Confira a mensagem acima.' }
} finally { $env:SUPABASE_PASSWORD = $previous }
