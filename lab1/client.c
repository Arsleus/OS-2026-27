#include <unistd.h>     
#include <fcntl.h>      
#include <sys/wait.h>   

int my_strlen(const char *str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

void print_string(const char *str) {
    write(STDOUT_FILENO, str, my_strlen(str));
}

int main() {
    char filename[256];
    
    print_string("Введите имя файла: ");
    
    int bytes_read = read(STDIN_FILENO, filename, sizeof(filename) - 1);
    if (bytes_read <= 0) {
        write(STDERR_FILENO, "Ошибка чтения\n", 14);
        return 1;
    }
    
    for (int i = 0; i < bytes_read; i++) {
        if (filename[i] == '\n' || filename[i] == '\r') {
            filename[i] = '\0';
            break;
        }
    }
    filename[bytes_read] = '\0';

    int file_fd = open(filename, O_RDONLY);
    if (file_fd < 0) {
        write(STDERR_FILENO, "Ошибка: не удалось открыть файл\n", 57);
        return 1;
    }

    print_string("Файл успешно открыт\n");

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        write(STDERR_FILENO, "Ошибка pipe\n", my_strlen("Ошибка pipe\n"));
        close(file_fd);
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        write(STDERR_FILENO, "Ошибка fork\n", my_strlen("Ошибка fork\n"));
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        close(file_fd);
        return 1;
    }

    if (pid == 0) {
        close(pipe_fd[0]);

        dup2(file_fd, STDIN_FILENO);
        close(file_fd);

        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[1]);

        char *args[] = {"./server", NULL};
        execv(args[0], args);

        _exit(1); 
    } 
    else {
        close(file_fd);
        close(pipe_fd[1]); 

        char buffer[1024];
        int bytes;

        print_string("Результаты от сервера\n");

        while ((bytes = read(pipe_fd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes] = '\0';
            print_string(buffer);
        }

        close(pipe_fd[0]);
        
        int status;
        wait(&status);
        
        if (WIFEXITED(status) && WEXITSTATUS(status) == 2) {
            write(STDERR_FILENO, "Обнаружено деление на ноль. Работа прервана.\n", my_strlen("Обнаружено деление на ноль. Работа прервана.\n"));
        }
    }
    return 0;
}
