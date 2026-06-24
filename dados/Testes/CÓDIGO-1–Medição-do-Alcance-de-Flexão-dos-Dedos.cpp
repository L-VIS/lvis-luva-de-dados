/**
 * @file flexao_dedos.ino
 * @brief Medição do alcance de flexão dos 5 dedos da luva (10 repetições por dedo)
 * 
 * Este código mede o ângulo máximo de flexão de cada dedo (polegar, indicador,
 * médio, anelar, mindinho) usando sensores flexíveis. O usuário deve fechar
 * a mão completamente e o código registra o ângulo (0° = aberto, 90° = fechado).
 * 
 * @author L-VIS / UnB
 * @date Junho/2026
 * 
 * @section Saída Serial
 * Exibe tabela com 10 repetições por dedo e a média/desvio padrão.
 * 
 * @section Pinagem
 * - Polegar:  porta 3 (digital)  - ATENÇÃO: verificar se é realmente digital
 * - Indicador: A0
 * - Médio:    A1
 * - Anelar:   A2
 * - Mindinho: A3
 */

// ========================================================================
//  MACROS DE CALIBRAÇÃO ESTÁTICA (valores brutos do ADC para cada dedo)
// ========================================================================

#define CAL_ABERTO_POLEGAR   0
#define CAL_FECHADO_POLEGAR  1

#define CAL_ABERTO_INDICADOR   450
#define CAL_FECHADO_INDICADOR  600

#define CAL_ABERTO_MEDIO   550
#define CAL_FECHADO_MEDIO  617

#define CAL_ABERTO_ANELAR   575
#define CAL_FECHADO_ANELAR  657

#define CAL_ABERTO_MINIMO   571
#define CAL_FECHADO_MINIMO  596

// ========================================================================
//  ESTRUTURA DE DADOS PARA UM DEDO
// ========================================================================

struct Dedo {
  uint8_t porta;          // Pino analógico (ou digital para polegar)
  String  nome;           // Nome do dedo
  int     leitura;        // Último valor lido
  int     angulo;         // Ângulo estimado (0° = aberto, 90° = fechado)
  int     aberto;         // Valor calibrado para o dedo esticado
  int     fechado;        // Valor calibrado para o dedo flexionado
};

Dedo dedos[5]; // Índices: 0=polegar, 1=indicador, 2=médio, 3=anelar, 4=mindinho

// ========================================================================
//  PROTÓTIPOS
// ========================================================================

int  calcularAngulo(int leitura, int fechado, int aberto, bool saturar = true);
void lerSensores();
void configurarDedos();

// ========================================================================
//  SETUP
// ========================================================================

void setup() {
  Serial.begin(4800);
  
  // Configuração dos dedos: portas, nomes e calibração estática
  configurarDedos();
  
  // Aguarda o usuário se preparar
  Serial.println("\n=== MEDIÇÃO DE ALCANCE DE FLEXÃO DOS DEDOS ===");
  Serial.println("Instruções:");
  Serial.println("1. Vista a luva.");
  Serial.println("2. Mantenha a mão completamente ABERTA por 3 segundos.");
  Serial.println("3. Em seguida, feche a mão (punho fechado) por 3 segundos.");
  Serial.println("4. O código registrará o ângulo de cada dedo.");
  Serial.println("5. O procedimento será repetido 10 vezes para cada dedo.");
  Serial.println("\nPressione qualquer tecla no monitor serial para iniciar...");
  while (!Serial.available()) { delay(100); }
  Serial.read();
  
  // Executa a medição
  medirFlexao();
}

// ========================================================================
//  LOOP
// ========================================================================

void loop() {
  // Nada a fazer – a medição é executada uma vez no setup
  delay(1000);
}

// ========================================================================
//  FUNÇÕES
// ========================================================================

void configurarDedos() {
  dedos[0].porta   = 3;
  dedos[0].nome    = "Polegar";
  dedos[0].aberto  = CAL_ABERTO_POLEGAR;
  dedos[0].fechado = CAL_FECHADO_POLEGAR;
  
  dedos[1].porta   = A0;
  dedos[1].nome    = "Indicador";
  dedos[1].aberto  = CAL_ABERTO_INDICADOR;
  dedos[1].fechado = CAL_FECHADO_INDICADOR;
  
  dedos[2].porta   = A1;
  dedos[2].nome    = "Médio";
  dedos[2].aberto  = CAL_ABERTO_MEDIO;
  dedos[2].fechado = CAL_FECHADO_MEDIO;
  
  dedos[3].porta   = A2;
  dedos[3].nome    = "Anelar";
  dedos[3].aberto  = CAL_ABERTO_ANELAR;
  dedos[3].fechado = CAL_FECHADO_ANELAR;
  
  dedos[4].porta   = A3;
  dedos[4].nome    = "Mindinho";
  dedos[4].aberto  = CAL_ABERTO_MINIMO;
  dedos[4].fechado = CAL_FECHADO_MINIMO;
  
  for (int i = 0; i < 5; i++) {
    pinMode(dedos[i].porta, INPUT);
  }
}

void lerSensores() {
  dedos[0].leitura = digitalRead(dedos[0].porta);
  for (int i = 1; i < 5; i++) {
    dedos[i].leitura = analogRead(dedos[i].porta);
  }
}

int calcularAngulo(int leitura, int fechado, int aberto, bool saturar) {
  int angulo = 90 * (leitura - aberto) / (fechado - aberto);
  if (saturar) {
    if (angulo > 90)  angulo = 90;
    if (angulo < 0)   angulo = 0;
  }
  return angulo;
}

void medirFlexao() {
  // Arrays para armazenar as 10 repetições de cada dedo
  const int NUM_REPETICOES = 10;
  int dados[NUM_REPETICOES][5]; // [repetição][dedo]
  
  Serial.println("\n=== INICIANDO MEDIÇÃO ===");
  Serial.println("Aguarde o prompt para cada repetição...\n");
  
  for (int rep = 0; rep < NUM_REPETICOES; rep++) {
    // Aguarda o usuário fechar a mão
    Serial.print("Repetição ");
    Serial.print(rep + 1);
    Serial.println(": FECHE a mão (punho fechado) e aguarde 3 segundos...");
    delay(3000);
    
    // Leitura com a mão fechada
    lerSensores();
    for (int d = 0; d < 5; d++) {
      dados[rep][d] = calcularAngulo(dedos[d].leitura,
                                     dedos[d].fechado,
                                     dedos[d].aberto,
                                     true);
    }
    
    // Exibe os valores lidos
    Serial.print("  Valores: ");
    for (int d = 0; d < 5; d++) {
      Serial.print(dedos[d].nome);
      Serial.print(": ");
      Serial.print(dados[rep][d]);
      Serial.print("°");
      if (d < 4) Serial.print(" | ");
    }
    Serial.println();
    
    // Aguarda o usuário abrir a mão para a próxima repetição
    if (rep < NUM_REPETICOES - 1) {
      Serial.println("  ABRA a mão e aguarde 2 segundos para a próxima repetição...");
      delay(2000);
    }
  }
  
  // ========================================================================
  //  EXIBIÇÃO DOS RESULTADOS
  // ========================================================================
  Serial.println("\n\n=== RESULTADOS FINAIS (10 repetições por dedo) ===");
  
  // Cabeçalho da tabela
  Serial.print("Repetição");
  for (int d = 0; d < 5; d++) {
    Serial.print(" | ");
    Serial.print(dedos[d].nome);
  }
  Serial.println();
  
  // Linhas de dados
  for (int rep = 0; rep < NUM_REPETICOES; rep++) {
    Serial.print(rep + 1);
    for (int d = 0; d < 5; d++) {
      Serial.print(" | ");
      Serial.print(dados[rep][d]);
    }
    Serial.println();
  }
  
  // Média e desvio padrão
  Serial.println("\n--- ESTATÍSTICAS ---");
  for (int d = 0; d < 5; d++) {
    // Calcula média
    float soma = 0;
    for (int rep = 0; rep < NUM_REPETICOES; rep++) {
      soma += dados[rep][d];
    }
    float media = soma / NUM_REPETICOES;
    
    // Calcula desvio padrão amostral
    float variancia = 0;
    for (int rep = 0; rep < NUM_REPETICOES; rep++) {
      variancia += pow(dados[rep][d] - media, 2);
    }
    float desvio = sqrt(variancia / (NUM_REPETICOES - 1));
    
    Serial.print(dedos[d].nome);
    Serial.print(": média ");
    Serial.print(media, 1);
    Serial.print("° ± ");
    Serial.print(desvio, 1);
    Serial.println("°");
  }
  
  Serial.println("\n=== FIM DA MEDIÇÃO ===");
}