#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "hardware/pio.h" //Bibloteca PIO para controle da matriz de leds
#include "hardware/clocks.h"
#include "hardware/adc.h" // Biblioteca para controle do ADC (Conversor Analógico-Digital).
#include "pico/binary_info.h"
#include "inc/ssd1306.h"  //Biblioteca de apoio ao controle do OLED
#include "hardware/i2c.h" //Comandos I2C

// Biblioteca gerada pelo arquivo .pio durante compilação.
#include "ws2818b.pio.h"

// Definição do número de LEDs e pino para a matriz de leds.
#define LED_COUNT 25
#define LED_PIN 7

// Pino do Buzzer. Pela notação do BitDogLab é o Buzzer B.
#define BUZZER_B_PIN 10

// Definições dos pinos para o joystick e botão
#define VRX_PIN 26 // Define o pino 26 para o eixo X do joystick (Canal ADC0).
#define VRY_PIN 27 // Define o pino 27 para o eixo Y do joystick (Canal ADC1).
#define SW_PIN 22  // Define o pino 22 para o botão do joystick (entrada digital).

const uint I2C_SDA = 14; // Define o pino 14 para os dados I2C
const uint I2C_SCL = 15; // Define o pino 15 para o clock I2C

// Definição de pixel GRB
struct pixel_t
{
    uint8_t G, R, B; // Três valores de 8-bits compõem um pixel.
};

typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t; // Mudança de nome de "struct pixel_t" para "npLED_t" por clareza.

// Declaração do buffer de pixels que formam a matriz.
npLED_t leds[LED_COUNT];

// Variáveis para uso da máquina PIO.
PIO np_pio;
uint sm;

int number_leds[15] = {2, 3, 4, 5, 6, 7, 12, 13, 14, 15, 16, 17, 22, 23, 24}; // Leds da matriz de leds que serão usados para as letras da matriz de leds - 3 colunas da esquerda
int dir_x = 0, dir_y = 0;                                                     // Variáveis de direção do joystick

// Estabelece a sequência de animação para mostrar cada letra
int nota_caractere[7][5][15] =
    {
        {// A
         {4, 2, 7, 12, 17},
         {6, 3, 6, 12, 13, 16, 22},
         {10, 2, 4, 5, 7, 12, 13, 14, 15, 17, 23},
         {6, 3, 6, 13, 14, 16, 24},
         {4, 4, 5, 14, 15}},
        {// B
         {5, 2, 7, 12, 17, 22},
         {8, 2, 3, 6, 12, 13, 16, 22, 23},
         {10, 3, 4, 5, 7, 13, 14, 15, 17, 23, 24},
         {5, 4, 6, 14, 16, 24},
         {2, 5, 15}},
        {// C
         {3, 7, 12, 17},
         {5, 2, 6, 13, 16, 22},
         {7, 2, 3, 5, 14, 15, 22, 23},
         {4, 3, 4, 23, 24},
         {2, 4, 24}},
        {// D
         {5, 2, 7, 12, 17, 22},
         {7, 2, 3, 6, 13, 16, 22, 23},
         {10, 3, 4, 5, 7, 12, 14, 15, 17, 23, 24},
         {5, 3, 6, 13, 16, 24},
         {3, 5, 14, 15}},
        {// E
         {5, 2, 7, 12, 17, 22},
         {8, 2, 3, 6, 12, 13, 16, 22, 23},
         {10, 2, 3, 4, 5, 13, 14, 15, 22, 23, 24},
         {5, 3, 4, 14, 23, 24},
         {2, 4, 24}},
        {// F
         {5, 2, 7, 12, 17, 22},
         {7, 3, 6, 12, 13, 16, 22, 23},
         {8, 4, 5, 13, 14, 15, 22, 23, 24},
         {3, 14, 23, 24},
         {1, 24}},
        {// G
         {3, 7, 12, 17},
         {6, 2, 6, 12, 13, 16, 22},
         {10, 2, 3, 5, 7, 12, 13, 14, 15, 22, 23},
         {7, 3, 4, 6, 13, 14, 23, 24},
         {4, 4, 5, 14, 24}}};

// Estrutura para armazenar as notas e seus períodos
typedef struct
{
    char nome[4]; // Nome da nota (ex: "C#-", "Db-")
    int periodo;  // Período em microssegundos
} Nota;

// Preparar área de renderização para o display oled (ssd1306_width pixels por ssd1306_n_pages páginas)
struct render_area frame_area = {
    start_column : 0,
    end_column : ssd1306_width - 1,
    start_page : 0,
    end_page : ssd1306_n_pages - 1
};

// Tabela de notas com o período de cada uma em microssegundos
Nota notas[] = {
    {"A-", 9091}, {"A#-", 8581}, {"Bb-", 8581}, {"B-", 8099}, {"C-", 7645}, {"C#-", 7215}, {"Db-", 7215}, {"D-", 6810}, {"D#-", 6428}, {"Eb-", 6428}, {"E-", 6067}, {"F-", 5727}, {"F#-", 5405}, {"Gb-", 5405}, {"G-", 5102}, {"G#-", 4816}, {"Ab", 4816}, {"A", 4545}, {"A#", 4290}, {"Bb", 4290}, {"B", 4050}, {"C", 3822}, {"C#", 3608}, {"Db", 3608}, {"D", 3405}, {"D#", 3214}, {"Eb", 3214}, {"E", 3034}, {"F", 2863}, {"F#", 2703}, {"Gb", 2703}, {"G", 2551}, {"G#", 2408}, {"Ab+", 2408}, {"A+", 2273}, {"A#+", 2145}, {"Bb+", 2145}, {"B+", 2025}, {"C+", 1911}, {"C#+", 1804}, {"Db+", 1804}, {"D+", 1703}, {"D#+", 1607}, {"Eb+", 1607}, {"E+", 1517}, {"F+", 1432}, {"F#+", 1351}, {"Gb+", 1351}, {"G+", 1276}, {"G#+", 1204}, {"Ab+", 1204}, {"A+", 1136}, {"A#+", 1073}, {"Bb+", 1073}, {"B+", 1012}};

char *nome_notas[] = {"A", "B", "C", "D", "E", "F", "G"}; // Nome das notas principais
char *musica[30];                                         // Alocação do array de músicas - terá 30 notas no máximo
bool high = false;                                        // Indicador de nível alto para vibração do buzzer
bool freq_up = true;                                      // Indicador de aumento de frequencia para vibração do buzzer
float count = 0.0;                                        // Contador para vibração do buzzer
int nota_atual = 500;                                     // Inicialização da variável que receberá o período da nota a tocas
alarm_id_t buzzer_alarm_id;                               // ID do timer para permitir cancelamento
bool escreveu_tela = false;                               // Indica que deverá escrever no display
float key = 0.0;                                          // Posição da nota. Varia entre 0 para A e 1 para G na parte inteira e .0 para tom e .5 para semitom na parte decimal
char nota_final[3];                                       // Armazena o texto da nota inteira, podendo ser composto pela nota (A...G), semitom (b/#) e oitava (-/+, nada para oitava intermediária)
int oitava = 0;                                           //-1 para oitava inferior, 0 para oitava intermediária e 1 para oitava superior
int nota = 0;                                             // Armazena o número total de notas de uma música

// Inicializa a máquina PIO para controle da matriz de LEDs.
void npInit(uint pin)
{
    // Cria programa PIO.
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;

    // Toma posse de uma máquina PIO.
    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0)
    {
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true); // Se nenhuma máquina estiver livre, panic!
    }

    // Inicia programa na máquina PIO obtida.
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

    // Limpa buffer de pixels.
    for (uint i = 0; i < LED_COUNT; ++i)
    {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
}

// Atribui uma cor RGB a um LED.
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b)
{
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

// Limpa toda a matriz
void npClear()
{
    for (uint i = 0; i < LED_COUNT; ++i)
        npSetLED(i, 0, 0, 0);
}

// Limpa apenas o espaço do número na matriz de leds
void npClearNumber()
{
    for (uint i = 0; i < 15; ++i)
        npSetLED(number_leds[i], 0, 0, 0);
}

// Escreve os dados do buffer nos LEDs
void npWrite()
{
    // Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
    for (uint i = 0; i < LED_COUNT; ++i)
    {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
}

// Função para buscar o período de uma nota
int buscarNota(const char *nota)
{
    int tamanho = sizeof(notas) / sizeof(notas[0]);
    for (int i = 0; i < tamanho; i++)
    {
        if (strcmp(notas[i].nome, nota) == 0)
        {
            return notas[i].periodo;
        }
    }
    return -1; // Retorna -1 se a nota não for encontrada
}

// Função de callback que gera oscilação no buzzer para ele tocar a nota desejada
int64_t buzzer_callback(alarm_id_t id, void *user_data)
{
    if (high)
    {
        high = false;
        gpio_put(BUZZER_B_PIN, 1);
    }
    else
    {
        high = true;
        gpio_put(BUZZER_B_PIN, 0);
        if (freq_up)
            count -= 0.01;
        else
            count += 0.01;
    }
    return nota_atual;
}

// Reinicia buzzer para tocar
void setup_buzzer_timer()
{
    if (buzzer_alarm_id)
        cancel_alarm(buzzer_alarm_id); // Cancela timer antigo
    buzzer_alarm_id = add_alarm_in_us(nota_atual, buzzer_callback, NULL, true);
}

// Escreve a nota tocada no display
void escreve_display(char *nota_final)
{
    // Apaga display
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

    // Reescreve display com a nota única
    char *text2[] = {
        nota_final};

    int y = 32;
    for (uint i = 0; i < count_of(text2); i++)
    {
        ssd1306_draw_string(ssd, 5, y, text2[i]);
        y += 8;
    }

    render_on_display(ssd, &frame_area);
}

// Escreve a música inteira no display
void musica2display(char *notas[], int num_notas)
{
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

    int y = 0; // Começar na linha superior
    int x = 0;

    for (int i = 0; i <= num_notas; i++)
    {
        if (i > 0)
        {
            x += 8 * (strlen(notas[i - 1])) + 4;   // Posiciona em x o caractere no display, pega o tamanho da nota e acrescenta 4px de espaço
            if (x >= (128 - strlen(notas[i - 1]))) // Se a posição do x mais o tamanho do caractere for passar de 128
            {
                x = 0; // Volta o cursor no início da linha
                y++;   // Pula a linha
            }
        }

        ssd1306_draw_string(ssd, x, y * 8, notas[i]); // Escreve no display
    }

    render_on_display(ssd, &frame_area);
}

// Lê o array com o as notas da música para tocar
void toca_musica(char *notas[], int num_notas)
{
    for (int i = 0; i < num_notas; i++)
    {
        nota_atual = buscarNota(notas[i]); // Pega o período da nota a se tocar
        sleep_ms(400);
        setup_buzzer_timer();
    }
}

int main()
{
    // Inicializa entradas e saídas.
    stdio_init_all();
    adc_init();
    adc_gpio_init(VRX_PIN);                           // Joystick x
    adc_gpio_init(VRY_PIN);                           // Joystick y
    gpio_init(SW_PIN);                                // Botão do joystick
    gpio_set_dir(SW_PIN, GPIO_IN);                    // Coloca botão como input
    gpio_pull_up(SW_PIN);                             // Pull up para garantir positivo
    gpio_init(BUZZER_B_PIN);                          // Inicializa buzzer
    gpio_set_dir(BUZZER_B_PIN, GPIO_OUT);             // Coloca como saída
    npInit(LED_PIN);                                  // Inicializa matriz de LEDs NeoPixel.
    npClear();                                        // Limpa a matriz
    sleep_ms(1000);                                   // Aguarda 1s para processamento
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);         // Inicializa o baud rate do display oled
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);        // Define pino para os dados
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);        // Define pino para o clock
    gpio_pull_up(I2C_SDA);                            // Joga pino de dados em alta
    gpio_pull_up(I2C_SCL);                            // Joga pino de clockem alta
    ssd1306_init();                                   // Processo de inicialização completo do OLED SSD1306
    calculate_render_area_buffer_length(&frame_area); // Calcular quanto do buffer será destinado à área de renderização
    // Limpa display
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

    // Apresenta o nome do aplicativo
    char *text[] = {
        "  ALSongs   ",
        "            ",
        "  by Luctech   "};

    int y = 16;
    for (uint i = 0; i < count_of(text); i++)
    {
        ssd1306_draw_string(ssd, 5, y, text[i]); // Linha 2 (2*8 pixels)
        y += 8;
    }
    render_on_display(ssd, &frame_area);

    while (true)
    {
        // Lê os valores de X e Y do joystick
        adc_select_input(0);
        uint16_t vrx_value = adc_read();
        adc_select_input(1);
        uint16_t vry_value = adc_read();
        

        // Toda vez que entrar um sleep, neste caso para processar o joystick, precisa silenciar o buzzer, sinal gera som indesejado
        cancel_alarm(buzzer_alarm_id);
        int sw_js = gpio_get(SW_PIN); // Pega valor do botão. Quando clicado, é 0.
        sleep_ms(100);
        setup_buzzer_timer();

        if (sw_js == 0) // Apertou o botão, toca a música
        {
            toca_musica(musica, nota);
        }

        // Estabelece as direções obtidas pelo joystick em função dos valores gerados pela leitora ADC.
        // Não foram usados os valores máximo e mínimo, mas uma faixa para deixar mais sensível ao toque
        if (vrx_value > 3500)
            dir_x = 1;
        else if (vrx_value < 1500)
            dir_x = -1;
        else
            dir_x = 0;

        if (vry_value > 3500)
            dir_y = 1;
        else if (vry_value < 1500)
            dir_y = -1;
        else
            dir_y = 0;

        // Na posição da placa BitDogLab para a visualização adequada pelo usuário, as direções são: direita(y==1), esqueda(y==-1),cima(x==1), baixo(x==-1)
        if (dir_x == 0)
        {
            if (dir_y == 1) // Para a direita. A nota aumenta como em um teclado de verdade
            {
                escreveu_tela = true; // Indica que deverá escrever no display

                if (key <= 6.0) // Key varia entre A e G (0 a 6). 0.5 para semitom e 1.0 para tom inteiro. Enquanto não for G...
                {
                    if ((key != 1.0) && (key != 4.0)) // Se não for B ou E, inclui semitom
                        key += 0.5;
                    else
                        key += 1.0; // Ou inclui tom inteiro
                }
                else if (oitava <= 0) // Se era a oitava intermediária ou a menor, depois de G#, volta a A, mas aumenta uma oitava
                {
                    key = 0.0;
                    oitava++;
                }
                // Escreve a nota na matriz de leds
                for (int j = 0; j < 3; j++)
                {
                    npClearNumber();
                    for (int i = 1; i <= nota_caractere[(int)key][j][0]; i++)
                        npSetLED(nota_caractere[(int)key][j][i], 10, 10, 10);

                    npWrite();
                    sleep_ms(100);
                }
            }

            else if (dir_y == -1) // Se o joystick for para a esquerda, faz o processo contrário de aumentar.
            {
                escreveu_tela = true;
                if (key > 0.0) // Enquanto foi maior que A...
                {
                    if ((key != 2.0) && (key != 5.0)) // Se não for C ou F, diminui semitom. Senão diminui um tom.
                    {
                        key -= 0.5;
                    }
                    else
                    {
                        key -= 1.0;
                    }
                }
                else if (oitava >= 0) // Se for uma oitava alta ou intermediária, permite reduzir uma oitava
                {
                    key = 6.5;
                    oitava--;
                }
                // Escreve a nota na matriz de leds
                for (int j = 5; j >= 2; j--)
                {
                    npClearNumber();
                    for (int i = nota_caractere[(int)key][j][0]; i >= 1; i--)
                        npSetLED(nota_caractere[(int)key][j][i], 10, 10, 10);
                    npWrite();
                    sleep_ms(100);
                }
            }

            strcpy(nota_final, nome_notas[(int)key]);
            if ((key - (int)key) == 0.5) // Se key tiver 5 décimos, considera como sustenido e inclui o #. Acende o led 21, indicando.
            {
                strcat(nota_final, "#");
                npSetLED(21, 150, 20, 150);
            }
            else
                npSetLED(21, 0, 0, 0);
            if (oitava < 0) // Se a oitava for a menor, acende led 18.
            {
                strcat(nota_final, "-");
                npSetLED(18, 150, 150, 20);
            }
            else if (oitava > 0) // Se a oitava for a maior, acende led 19
            {
                strcat(nota_final, "+");
                npSetLED(19, 150, 150, 20);
            }
            else
            {
                npSetLED(18, 0, 0, 0);
                npSetLED(19, 0, 0, 0);
            }

            npWrite();

            nota_atual = buscarNota(nota_final); // Busca o período pelo nome da nota
            sleep_us(200);
            if (escreveu_tela)
            {
                escreve_display(nota_final); // Escreve a nota na tela
                escreveu_tela = false;       // Bloqueia escrita
            }

            setup_buzzer_timer(); // Toca nota
        }

        else if (dir_x == 1) // Se o joystick for para cima, inclui uma nota
        {
            cancel_alarm(buzzer_alarm_id);
            musica[nota] = strdup(nota_final); // Inclui nota

            musica2display(musica, nota); // Mostra a música inteira no display
            sleep_ms(200);
            nota++;
            setup_buzzer_timer(); // Toca a nota incluída
        }

        else if (dir_x == -1) // Se o joystick for para cima, inclui uma nota
        {
            cancel_alarm(buzzer_alarm_id);
            if (nota > 0)
            {
                nota--;              // Move o índice para trás
                free(musica[nota]);  // Libera a memória da última nota
                musica[nota] = NULL; // Remove a referência da nota                
            }

            musica2display(musica, nota); // Mostra a música inteira no display
            
            sleep_ms(200);
        }
    }
}
