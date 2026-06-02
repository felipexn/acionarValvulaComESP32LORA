# acionarValvulaComESP32LORA
Repositorio para mostrar a linha do tempo do projeto que consiste em acionar uma valvula solenoide com um ESP32 LoRa.

## Quarto Avanco

Nesta pasta estao as tres versoes usadas no quarto avanco do projeto:

- `principalteste.ino`: painel web + sincronizacao de hora + envio do comando
- `acionador.ino`: recebe o comando e executa a rotina da valvula e da bomba
- `senderumidade3.ino`: envia a umidade do solo via LoRa

## Quinto Avanco Refatorizado

Esta pasta contem a versao refatorada do quarto avanco, mantendo os mesmos tres sketches:

- `principalteste.ino`: painel web + sincronizacao de hora + envio do comando
- `acionador.ino`: recebe o comando e executa a rotina da valvula e da bomba
- `senderumidade3.ino`: envia a umidade do solo via LoRa

## Protocolo LoRa

- `H:<valor>` vem do sensor de umidade. Exemplo: `H:43`
- `A<opcao>` vem do painel web para disparar a rotina. Exemplo: `A1`
- `OK` e a resposta enviada pelo acionador

Os sketches ainda aceitam os formatos antigos `umidade:<valor>%` e `acionar##<opcao>` para facilitar testes durante a transicao.

## Configuracao LoRa

- Pacotes curtos: `H:<valor>`, `A<opcao>` e `OK`.
- Envio do sensor: mudanca minima de 2 pontos ou intervalo maximo de 10 minutos.
- Leitura do sensor: a cada 30 segundos.
- Potencia de transmissao: 10 dBm no sensor e 14 dBm nos nos de comando.
- Modo dos receptores: recepcao reativada depois de cada transmissao.


