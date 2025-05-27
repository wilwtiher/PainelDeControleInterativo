# Aluno: Wilton Lacerda Silva Júnior
## Matrícula: TIC370100193
# Video explicativo: https://youtu.be/PoTmVbJsmQU
# Painel de Controle Interativo
O objetivo do projeto é aprimorar o conhecimento do FreeRTOS utilizando semáforos em um projeto de contagem.
## Funcionalidades

- **Display OLED**
  - O display mostrará o estado da variável de contagem e o que aconteceu no programa.
- **3 Botões**
  - O botão B está sendo utilizado com a função de subtrair.
  - O botão A está sendo utilizado com a função de adicionar.
  - O botão do Joystick está sendo utilizado com a função de reiniciar.
- **BUZZER**
  - O buzzer servirá como identificação sonora.
- **LED RGB**
   - O LED RGB do meio da placa mostrará o estado da variável de contagem em 4 estados.

# Requisitos
## Hardware:

- Raspberry Pi Pico W.
- 1 display ssd1306 com o sda na porta 14 e o scl na porta 15.
- 1 led rgb, com o led vermelho no pino 13 e o led verde no pino 11 e o pino azul no 12.
- 3 botões, um no pino 5, outro no pino 6 e o ultimo no 22.
- 1 buzzer no pino 10.

## Software:

- Ambiente de desenvolvimento VS Code com extensão Pico SDK.
- FreeRTOS configurado para a sua máquina.

# Instruções de uso
## Configure o ambiente:
- Certifique-se de que o Pico SDK está instalado e configurado no VS Code.
- Configure o FreeRTOS para a configuração da sua máquina.
- Compile o código utilizando a extensão do Pico SDK.
## Teste:
- Utilize a placa BitDogLab para o teste. Caso não tenha, conecte os hardwares informados acima nos pinos correspondentes.

# Explicação do projeto:
## Contém:
- O projeto terá 3 meios de entrada: Os botões.
- Também contará com saídas visuais e sonoras, sendo o led rgb no meio da placa, o buzzer, e o display OLED.

## Funcionalidades:
- O programa mostrará uma representação de uma contagem de pessoas.
- O programa mostrará por meio do led central uma representação da lotação.
- O buzzer representará sonoramente caso alguma função for apertada.
- O display OLED mostrará em qual estádo a contagem de pessoas está.
- Os botões acrescentará, subtrairá, ou reiniciará o programa, em questão da variável.
