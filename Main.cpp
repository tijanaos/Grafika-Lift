#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <thread>
#include <chrono>

#include "Util.h"

// Globalna promenljiva za deltaTime (u sekundama)
float deltaTime = 0.0f;

// Pomoćna funkcija za lepo gašenje programa sa porukom
int endProgram(std::string message) {
    std::cout << message << std::endl;
    glfwTerminate();
    return -1;
}

int main()
{
    // Inicijalizacija GLFW i postavljanje na verziju 3 sa programabilnim pajplajnom
    if (!glfwInit()) {
        return endProgram("GLFW nije uspeo da se inicijalizuje.");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // FULLSCREEN: uzmi primarni monitor i njegov video mode
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (!monitor) {
        return endProgram("Nisam uspeo da dobijem primarni monitor.");
    }

    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    if (!mode) {
        return endProgram("Nisam uspeo da dobijem video mode.");
    }

    int screenWidth = mode->width;
    int screenHeight = mode->height;

    // Formiranje prozora preko celog ekrana
    GLFWwindow* window = glfwCreateWindow(
        screenWidth,
        screenHeight,
        "Lift projekat",
        monitor,          // FULLSCREEN: ovde prosledjujemo monitor
        nullptr
    );
    if (window == NULL) {
        return endProgram("Prozor nije uspeo da se kreira.");
    }

    glfwMakeContextCurrent(window);

    // Inicijalizacija GLEW
    if (glewInit() != GLEW_OK) {
        return endProgram("GLEW nije uspeo da se inicijalizuje.");
    }

    // Podesi viewport da pokrije ceo ekran
    glViewport(0, 0, screenWidth, screenHeight);

    // Boja pozadine (ovo možemo kasnije menjati)
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    // FPS limiter
    const double TARGET_FPS = 75.0;
    const double TARGET_FRAME_TIME = 1.0 / TARGET_FPS; // ~0.013333s po frejmu

    // GLAVNA PETLJA
    while (!glfwWindowShouldClose(window))
    {
        // Vreme pocetka frejma
        double frameStartTime = glfwGetTime();

        // ESC treba da ugasi program
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        // Čišćenje bafera pre crtanja
        glClear(GL_COLOR_BUFFER_BIT);

        // (Za sada ništa ne crtamo – i dalje je crn ekran)

        // Zamena bafera i obrada događaja
        glfwSwapBuffers(window);
        glfwPollEvents();

        // Koliko je ovaj frejm trajao do sada
        double frameEndTime = glfwGetTime();
        double frameDuration = frameEndTime - frameStartTime;

        // Ako je frejm bio brži od TARGET_FRAME_TIME, uspavaj nit
        if (frameDuration < TARGET_FRAME_TIME) {
            double sleepTime = TARGET_FRAME_TIME - frameDuration;
            // sleepTime je u sekundama (double), pretvara se u chrono trajanje
            std::this_thread::sleep_for(std::chrono::duration<double>(sleepTime));

            // Posle spavanja, ažuriraj vreme kraja i trajanje frejma
            frameEndTime = glfwGetTime();
            frameDuration = frameEndTime - frameStartTime;
        }

        // Na kraju frejma upiši deltaTime (u sekundama)
        deltaTime = static_cast<float>(frameDuration);

        // (Opcija za debug: recimo da ispišeš FPS jednom u par sekundi)
        // std::cout << "FPS: " << 1.0 / frameDuration << "\n";
    }

    glfwTerminate();
    return 0;
}
