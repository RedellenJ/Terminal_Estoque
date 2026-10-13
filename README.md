# Doc'ê Hamburgueria — Gestão financeira com Supabase

Aplicação C++ de terminal para registrar entradas, saídas, saldo e extratos. As movimentações são armazenadas exclusivamente no PostgreSQL do Supabase. Os valores continuam em centavos inteiros. É necessária conexão com a internet: não existe gravação local alternativa nem fila offline.

## Executar

O banco ja esta configurado e o historico anterior foi importado. Abra `dist/DoceHamburgueria-Windows.exe` e informe sua senha, ou execute:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\iniciar.ps1
```

A senha nao aparece na tela e fica apenas em memoria. O arquivo `dist/supabase.ps1` deve permanecer ao lado do executavel. A configuracao real fica no `.env` da raiz, ignorado pelo Git, e e copiada para `dist/.env` ao compilar. Para instalar em outro computador, leve o `.env` junto com o executavel e o auxiliar. Variaveis de ambiente tem prioridade sobre o `.env`.

## Uso e acesso

Registre o saldo inicial como entrada. Informe valores como `150,50` ou `150.50`, sem separadores de milhar. Saídas que deixam o saldo negativo exigem confirmação. O servidor valida os limites e serializa as gravações da mesma conta para evitar inconsistências entre instâncias. Se o saldo mudar durante a confirmação, uma saída sem autorização de saldo negativo é recusada.

Cada usuário de Authentication tem seu próprio histórico. Para visualizar o mesmo caixa em computadores diferentes, use o mesmo usuário. As regras de RLS impedem que outros usuários leiam esse histórico, e as gravações só podem ocorrer pelas funções autenticadas. A exclusão remove apenas o histórico da conta conectada, exige `s` e depois `APAGAR`, e dispensa a antiga senha fixa `1234`.

O programa consulta o banco a cada retorno ao menu. Se uma operação falhar, ele informa o erro e encerra. Se a conexão cair durante uma gravação, confira o extrato ao reabrir antes de repetir: uma resposta perdida pode ocorrer depois que o banco confirmou a gravação.

A opção `[4]` gera **somente por solicitação** `extrato.csv` e `extrato.xml` no computador. São relatórios, não o armazenamento das movimentações. O CSV usa UTF-16 LE e `;`; o XML abre no Excel com cores e colunas ajustadas. A próxima exportação substitui os relatórios anteriores. Feche os arquivos no Excel antes de exportar ou excluir o histórico. `--dados pasta` escolhe apenas a pasta desses relatórios.

Este programa registra informações financeiras manualmente e não se conecta à conta bancária da hamburgueria.

## Compilação

Windows: com Zig 0.13.0 em `tools/zig-windows-x86_64-0.13.0`:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\compilar.ps1
```

A compilação gera os executáveis Windows e Linux e copia o auxiliar. No Windows, usa o Windows PowerShell já disponível no sistema. No Linux, instale **PowerShell 7 (`pwsh`)** e mantenha `supabase.ps1` ao lado do executável:

```sh
chmod +x dist/DoceHamburgueria-Linux
read -r -s -p 'Digite sua senha: ' SUPABASE_PASSWORD; echo
export SUPABASE_PASSWORD
./dist/DoceHamburgueria-Linux
unset SUPABASE_PASSWORD
```

Também é possível compilar no Linux com `g++ -std=c++17 -O2 src/main.cpp -o DoceHamburgueria`, copiando `src/supabase.ps1` para a pasta do executável. O binário C++ usa vinculação estática, mas a integração depende do PowerShell instalado.

Referencias: [chaves de API](https://supabase.com/docs/guides/api/api-keys), [RLS](https://supabase.com/docs/guides/database/postgres/row-level-security), [autenticação](https://supabase.com/docs/guides/auth).
