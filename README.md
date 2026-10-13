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
```

Registre o dinheiro que já existe na conta como uma entrada com descrição “Saldo inicial”. Informe valores como `150,50` ou `150.50`, sem separadores de milhar. Saídas que deixam o saldo negativo exigem confirmação adicional.

Os dados ficam em `movimentacoes.dat`, ao lado do executável. O CSV fica em `extrato.csv` no mesmo local. Preserve esse arquivo de dados e faça cópias de segurança. Abra apenas uma instância por arquivo de dados. Para escolher outra pasta, execute com `--dados caminho/da/pasta`.

Este é um controle manual: não se conecta ao banco. O saldo corresponde às movimentações registradas. A exportação substitui o CSV anterior.

A opção `[4] Exportar CSV / planilha Excel` gera `extrato.csv` em UTF-16 LE com identificação de codificação para preservar os acentos no Excel, e `extrato.xml`, uma planilha que abre no Excel com cabeçalho destacado, colunas dimensionadas e quebra de linha nas descrições. Abra `extrato.xml` para usar a formatação; você pode salvá-lo como `.xlsx` pelo Excel. Ambos são gerados ao lado dos dados e substituídos na próxima exportação. Feche os arquivos no Excel antes de exportar novamente.

O CSV inclui a indicação `sep=;` para abrir as cinco colunas corretamente. Valor e saldo são números com duas casas decimais. CSV não armazena largura de colunas: para ajustá-las no Excel, selecione A até E e dê dois cliques na divisão entre os cabeçalhos. Ao importar em outros programas, selecione UTF-16 LE, ignore a primeira linha `sep=;` e use o delimitador ponto e vírgula.

A opção `[5] Deletar todo o historico` apaga todas as movimentações e zera o saldo. O aviso “Você tem certeza que deseja deletar? Esse ato não pode ser revertido.” aparece em vermelho no terminal. Para apagar, digite `s` e depois a senha provisória `1234`. Qualquer outra resposta ou senha cancela a exclusão. O histórico apagado permanece vazio ao reabrir o programa. A exclusão também remove `extrato.csv` e `extrato.xml` da pasta dos dados. Se um deles estiver aberto no Excel, o programa informa que não conseguiu removê-lo; feche o arquivo e repita a exclusão.

## Compilar novamente

Com Zig 0.13.0 extraído em `tools/zig-windows-x86_64-0.13.0`, execute `./compilar.ps1` no PowerShell. No Linux com g++: `g++ -std=c++17 -O2 src/main.cpp -o DoceHamburgueria`.

O executável Linux usa musl com vinculação estática, sem precisar instalar bibliotecas C++.
