/* telnet.c – Telnet ns portu 23, ovladani LED a HTTP klientem */

#include "lwip/api.h"
#include "lwip/sys.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "main.h"   // piny LED

#define TELNET_PORT        23
#define CMD_BUFFER_LEN     64
#define HTTP_BUF_SIZE      1024

// staticke buffery – не грузим стек
static char g_cmd_buffer[CMD_BUFFER_LEN];
static char g_reply_buffer[128];
static char g_http_buffer[HTTP_BUF_SIZE];

static void telnet_thread(void *argument);
static void telnet_byte_available(uint8_t c, struct netconn *conn);
static void telnet_process_command(char *cmd, struct netconn *conn);
static void telnet_send(struct netconn *conn, const char *s);
static void http_client(char *s, uint16_t size);

// verejna inicializacni funkce (vola se z main.c) mela bych...
void telnet_init(void)
{
  const uint16_t TELNET_STACKSIZE = 2048;   // если будет мало – можно поднять (наверное :))
  sys_thread_new("telnet", telnet_thread, NULL,
                 TELNET_STACKSIZE,
                 osPriorityNormal);
}

// Telnet vlákno (server na portu 23)
static void telnet_thread(void *argument)
{
  struct netconn *conn, *newconn;
  err_t err;

  LWIP_UNUSED_ARG(argument);

  conn = netconn_new(NETCONN_TCP);
  if (conn == NULL) {
    return;
  }

  netconn_bind(conn, IP_ADDR_ANY, TELNET_PORT);
  netconn_listen(conn);

  for (;;) {
    err = netconn_accept(conn, &newconn);
    if (err == ERR_OK) {
      struct netbuf *buf;
      uint8_t *data;
      u16_t len;

      telnet_send(newconn, "Welcome to STM32 Telnet server\r\n");
      telnet_send(newconn, "Type HELP for commands.\r\n> ");

      while ((err = netconn_recv(newconn, &buf)) == ERR_OK) {
        do {
          netbuf_data(buf, (void **)&data, &len);
          for (u16_t i = 0; i < len; i++) {
            telnet_byte_available(data[i], newconn);
          }
        } while (netbuf_next(buf) >= 0);

        netbuf_delete(buf);
      }

      netconn_close(newconn);
      netconn_delete(newconn);
    }
  }
}

// skladani prikazu po znacich
static void telnet_byte_available(uint8_t c, struct netconn *conn)
{
  static uint16_t cnt = 0;

  if (c >= ' ' && c <= 127 && cnt < CMD_BUFFER_LEN - 1) {
    g_cmd_buffer[cnt++] = (char)c;
  }

  if (c == '\r' || c == '\n') {
    if (cnt > 0) {
      g_cmd_buffer[cnt] = '\0';
      telnet_process_command(g_cmd_buffer, conn);
      cnt = 0;
    }
    telnet_send(conn, "> ");
  }
}

// posílání textu klientovi
static void telnet_send(struct netconn *conn, const char *s)
{
  netconn_write(conn, s, strlen(s), NETCONN_COPY);
}

// HTTP klient – stáhne /ip.php z www.urel.feec.vutbr.cz
static void http_client(char *s, uint16_t size)
{
  struct netconn *client;
  struct netbuf  *buf;
  ip_addr_t ip;
  uint16_t len = 0;

  // IP adresa www.urel.feec.vutbr.cz = 147.229.144.124
  IP_ADDR4(&ip, 147, 229, 144, 124);

  const char *request =
      "GET /ip.php HTTP/1.1\r\n"
      "Host: www.urel.feec.vutbr.cz\r\n"
      "Connection: close\r\n"
      "\r\n";

  client = netconn_new(NETCONN_TCP);
  if (client == NULL) {
    snprintf(s, size, "Chyba: netconn_new\r\n");
    return;
  }

  if (netconn_connect(client, &ip, 80) == ERR_OK) {

    netconn_write(client, request, strlen(request), NETCONN_COPY);

    s[0] = '\0';

    while (len < size - 1 && netconn_recv(client, &buf) == ERR_OK) {
      void *data;
      u16_t blen;

      netbuf_data(buf, &data, &blen);

      if (len + blen > size - 1) {
        blen = size - 1 - len;
      }

      memcpy(&s[len], data, blen);
      len += blen;
      s[len] = '\0';

      netbuf_delete(buf);

      if (blen == 0) {
        break;
      }
    }
  } else {
    snprintf(s, size, "Chyba pripojeni\r\n");
  }

  netconn_close(client);
  netconn_delete(client);
}

// parser prikazu
static void telnet_process_command(char *cmd, struct netconn *conn)
{
  char *reply = g_reply_buffer;

  // prevod na velke pismena
  for (char *p = cmd; *p; ++p) {
    *p = (char)toupper((unsigned char)*p);
  }

  if (strcmp(cmd, "HELLO") == 0) {
    snprintf(reply, 128, "Hello from STM32 Nucleo!\r\n");
    telnet_send(conn, reply);
  }
  else if (strcmp(cmd, "STATUS") == 0) {
    GPIO_PinState l1 = HAL_GPIO_ReadPin(GPIOB, LD1_Pin);
    GPIO_PinState l2 = HAL_GPIO_ReadPin(GPIOB, LD2_Pin);
    GPIO_PinState l3 = HAL_GPIO_ReadPin(GPIOB, LD3_Pin);

    snprintf(reply, 128,
             "LED1=%s LED2=%s LED3=%s\r\n",
             (l1 == GPIO_PIN_SET) ? "ON" : "OFF",
             (l2 == GPIO_PIN_SET) ? "ON" : "OFF",
             (l3 == GPIO_PIN_SET) ? "ON" : "OFF");
    telnet_send(conn, reply);
  }
  else if (strncmp(cmd, "LED1 ", 5) == 0) {
    if (strstr(cmd + 5, "ON"))  HAL_GPIO_WritePin(GPIOB, LD1_Pin, GPIO_PIN_SET);
    if (strstr(cmd + 5, "OFF")) HAL_GPIO_WritePin(GPIOB, LD1_Pin, GPIO_PIN_RESET);
    telnet_send(conn, "OK\r\n");
  }
  else if (strncmp(cmd, "LED2 ", 5) == 0) {
    if (strstr(cmd + 5, "ON"))  HAL_GPIO_WritePin(GPIOB, LD2_Pin, GPIO_PIN_SET);
    if (strstr(cmd + 5, "OFF")) HAL_GPIO_WritePin(GPIOB, LD2_Pin, GPIO_PIN_RESET);
    telnet_send(conn, "OK\r\n");
  }
  else if (strncmp(cmd, "LED3 ", 5) == 0) {
    if (strstr(cmd + 5, "ON"))  HAL_GPIO_WritePin(GPIOB, LD3_Pin, GPIO_PIN_SET);
    if (strstr(cmd + 5, "OFF")) HAL_GPIO_WritePin(GPIOB, LD3_Pin, GPIO_PIN_RESET);
    telnet_send(conn, "OK\r\n");
  }
  else if (strcmp(cmd, "CLIENT") == 0) {
    // HTTP klient - stahne /ip.php a vypise vysledek
    http_client(g_http_buffer, HTTP_BUF_SIZE);
    telnet_send(conn, g_http_buffer);
  }
  else if (strcmp(cmd, "HELP") == 0) {
    telnet_send(conn,
      "Commands:\r\n"
      "  HELLO\r\n"
      "  STATUS\r\n"
      "  LED1 ON|OFF\r\n"
      "  LED2 ON|OFF\r\n"
      "  LED3 ON|OFF\r\n"
      "  CLIENT\r\n");
  }
  else {
    snprintf(reply, 128, "Unknown command: %s\r\n", cmd);
    telnet_send(conn, reply);
  }
}
