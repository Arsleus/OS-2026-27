#include <unistd.h>

int my_strlen(const char *str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

void int_to_str(int num, char *str) {
    int i = 0;
    int is_negative = 0;

    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    if (num < 0) {
        is_negative = 1;
        num = -num;
    }

    while (num > 0) {
        str[i++] = (num % 10) + '0';
        num = num / 10;
    }

    if (is_negative) {
        str[i++] = '-';
    }

    str[i] = '\0';

    int len = i;
    for (int j = 0; j < len / 2; j++) {
        char temp = str[j];
        str[j] = str[len - 1 - j];
        str[len - 1 - j] = temp;
    }
}

int main() {
    char ch;
    int current_num = 0;
    int first_in_line = 1; 
    int base_num = 0;      
    int has_digits = 0;    
    int sign = 1;

    while (read(STDIN_FILENO, &ch, 1) > 0) {
        if (ch == '-' && !has_digits) {
            sign = -1;
            continue;
        }

        if (ch >= '0' && ch <= '9') {
            current_num = current_num * 10 + (ch - '0');
            has_digits = 1;
        } 
        else if (ch == ' ' || ch == '\n' || ch == '\r') {
            if (has_digits) {
                current_num *= sign;

                if (first_in_line) {
                    base_num = current_num;
                    first_in_line = 0;
                } else {
                    if (current_num == 0) {
                        write(STDOUT_FILENO, "ERROR\n", 6);
                        return 2;
                    }
                    base_num /= current_num;
                }
                current_num = 0;
                has_digits = 0;
                sign = 1;
            }

            if (ch == '\n' || ch == '\r') {
                if (!first_in_line) {
                    char res_str[32];
                    int_to_str(base_num, res_str);
                    write(STDOUT_FILENO, res_str, my_strlen(res_str));
                    write(STDOUT_FILENO, "\n", 1);
                }
                first_in_line = 1;
            }
        }
    }

    if (has_digits && !first_in_line) {
        current_num *= sign;
        if (current_num == 0) {
            write(STDOUT_FILENO, "ERROR\n", 6);
            return 2;
        }
        base_num /= current_num;
        
        char res_str[32];
        int_to_str(base_num, res_str);
        write(STDOUT_FILENO, res_str, my_strlen(res_str));
        write(STDOUT_FILENO, "\n", 1);
    }

    return 0;
}
