param([switch] $Library)
$ErrorActionPreference = 'Stop'
$utf8 = New-Object System.Text.UTF8Encoding($false)
[Console]::InputEncoding = $utf8
[Console]::OutputEncoding = $utf8
$OutputEncoding = $utf8
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
$script:session = $null
$script:expires = [DateTime]::MinValue

function Initialize-Supabase {
    $config = Join-Path $PSScriptRoot '.env'
    if (Test-Path -LiteralPath $config) {
        foreach ($line in [IO.File]::ReadAllLines($config, $utf8)) {
            if ($line -match '^\s*(SUPABASE_[A-Z_]+)\s*=(.*)$') {
                $name = $matches[1]; $value = $matches[2].Trim()
                if ($value.Length -ge 2 -and (($value.StartsWith('"') -and $value.EndsWith('"')) -or ($value.StartsWith("'") -and $value.EndsWith("'")))) { $value = $value.Substring(1, $value.Length - 2) }
                if (![Environment]::GetEnvironmentVariable($name)) { [Environment]::SetEnvironmentVariable($name, $value, 'Process') }
            }
        }
    }
    foreach ($name in @('SUPABASE_URL', 'SUPABASE_PUBLISHABLE_KEY', 'SUPABASE_EMAIL', 'SUPABASE_PASSWORD')) {
        if (![Environment]::GetEnvironmentVariable($name)) { throw "Falta uma configuracao para entrar. Contate o responsavel pelo programa." }
    }
    $script:baseUrl = $env:SUPABASE_URL.TrimEnd('/')
    $uri = [Uri]$script:baseUrl
    if ($uri.Scheme -ne 'https' -or $uri.AbsolutePath -ne '/' -or $uri.Query -or $uri.UserInfo) { throw 'O endereco do servidor esta configurado incorretamente. Contate o responsavel pelo programa.' }
    if ($env:SUPABASE_PUBLISHABLE_KEY.StartsWith('sb_secret_')) { throw 'A configuracao de acesso esta incorreta. Contate o responsavel pelo programa.' }
    if ($env:SUPABASE_PUBLISHABLE_KEY.StartsWith('eyJ')) {
        try {
            $part = $env:SUPABASE_PUBLISHABLE_KEY.Split('.')[1].Replace('-', '+').Replace('_', '/')
            $part = $part.PadRight($part.Length + ((4 - $part.Length % 4) % 4), '=')
            $claims = [Text.Encoding]::UTF8.GetString([Convert]::FromBase64String($part)) | ConvertFrom-Json
            if ($claims.role -ne 'anon') { throw 'A configuracao de acesso esta incorreta.' }
        } catch { throw 'A configuracao de acesso esta incorreta. Contate o responsavel pelo programa.' }
    }
}

function Invoke-SupabaseHttp($path, $body, [switch] $Authenticated) {
    $headers = @{ apikey = $env:SUPABASE_PUBLISHABLE_KEY }
    if ($Authenticated) { $headers.Authorization = 'Bearer ' + $script:session.access_token }
    try {
        $bytes = $utf8.GetBytes(($body | ConvertTo-Json -Depth 12 -Compress))
        Invoke-RestMethod -Uri ($script:baseUrl + $path) -Method Post -Headers $headers -ContentType 'application/json; charset=utf-8' -Body $bytes -TimeoutSec 30
    } catch {
        $status = 0
        if ($_.Exception.Response) { $status = [int]$_.Exception.Response.StatusCode }
        if ($path.StartsWith('/auth/')) { throw "Nao foi possivel entrar. Confira seu email e sua senha." }
        $detail = ''
        if ($_.ErrorDetails.Message) {
            try { $errorBody = $_.ErrorDetails.Message | ConvertFrom-Json; $detail = [string]$errorBody.message } catch {}
        }
        if ($status -eq 0) { throw 'Sem conexao. Confira sua internet e tente novamente. Se estava salvando uma movimentacao, consulte o extrato antes de repetir.' }
        throw "Nao foi possivel concluir a operacao. $detail"
    }
}

function Connect-Supabase {
    if ($null -eq $script:session) {
        $script:session = Invoke-SupabaseHttp '/auth/v1/token?grant_type=password' @{ email = $env:SUPABASE_EMAIL; password = $env:SUPABASE_PASSWORD }
    } elseif ([DateTime]::UtcNow -lt $script:expires) { return }
    else {
        $script:session = Invoke-SupabaseHttp '/auth/v1/token?grant_type=refresh_token' @{ refresh_token = $script:session.refresh_token }
    }
    if (!$script:session.access_token) { throw 'Nao foi possivel entrar. Tente novamente.' }
    $script:expires = [DateTime]::UtcNow.AddSeconds([Math]::Max(1, [int]$script:session.expires_in - 60))
}

function Invoke-SupabaseRpc($name, $body) {
    Connect-Supabase
    Invoke-SupabaseHttp ('/rest/v1/rpc/' + $name) $body -Authenticated
}

function ConvertTo-LegacyQuoted([string] $value) {
    return '"' + $value.Replace('\', '\\').Replace('"', '\"') + '"'
}

if ($Library) { return }
try {
    Initialize-Supabase
    while ($null -ne ($command = [Console]::ReadLine())) {
        if ($command -eq 'QUIT') { break }
        try {
            $rows = @()
            if ($command -eq 'LOAD') {
                $snapshot = Invoke-SupabaseRpc 'finance_load' @{}
                $rows = @($snapshot.entries | Where-Object { $null -ne $_ })
            } elseif ($command.StartsWith('ADD ')) {
                $null = Invoke-SupabaseRpc 'finance_add' ($command.Substring(4) | ConvertFrom-Json)
            } elseif ($command -eq 'DELETE') {
                $null = Invoke-SupabaseRpc 'finance_clear' @{}
            } else { throw 'Comando desconhecido.' }
            [Console]::WriteLine('OK ' + $rows.Count)
            foreach ($row in $rows) {
                [Console]::WriteLine((ConvertTo-LegacyQuoted $row.date) + ' ' + (ConvertTo-LegacyQuoted $row.type) + ' ' + $row.value + ' ' + (ConvertTo-LegacyQuoted $row.description))
            }
        } catch { [Console]::WriteLine('ERROR ' + ($_.Exception.Message -replace '[\r\n]', ' ')) }
    }
} catch { [Console]::WriteLine('ERROR ' + ($_.Exception.Message -replace '[\r\n]', ' ')) }
