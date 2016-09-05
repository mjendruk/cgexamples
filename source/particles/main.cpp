
#include <iostream>

// C++ library for creating windows with OpenGL contexts and receiving 
// input and events http://www.glfw.org/ 
#include <GLFW/glfw3.h> 

// C++ binding for the OpenGL API. 
// https://github.com/cginternals/glbinding
#include <glbinding/Binding.h>

#include <cgutils/common.h>

#include "particles.h"


// From http://en.cppreference.com/w/cpp/language/namespace:
// "Unnamed namespace definition. Its members have potential scope 
// from their point of declaration to the end of the translation
// unit, and have internal linkage."
namespace
{

auto example = Particles();

const auto canvasWidth = 1440; // in pixel
const auto canvasHeight = 900; // in pixel

// "The size callback ... which is called when the window is resized."
// http://www.glfw.org/docs/latest/group__window.html#gaa40cd24840daa8c62f36cafc847c72b6
void resizeCallback(GLFWwindow * window, int width, int height)
{
    example.resize(width, height);
}

// "The key callback ... which is called when a key is pressed, repeated or released."
// http://www.glfw.org/docs/latest/group__input.html#ga7e496507126f35ea72f01b2e6ef6d155
void keyCallback(GLFWwindow * /*window*/, int key, int /*scancode*/, int action, int mods)
{

    switch (key)
    {
    case GLFW_KEY_LEFT:
        example.rotate(example.angle() + 0.02f);
        break;

    case GLFW_KEY_RIGHT:
        example.rotate(example.angle() - 0.02f);
        break;

    case GLFW_KEY_S:
        example.setScale(example.scale() + (mods & GLFW_MOD_SHIFT ? 1.f : -1.f));
        break;
    }

    if (action != GLFW_RELEASE)
        return;

    switch (key)
    {
    case GLFW_KEY_F5:
        example.loadShaders();
        break;

    case GLFW_KEY_F6:
        example.benchmark();
        std::cout << "Benchmark started" << std::endl;
        break;

    case GLFW_KEY_SPACE:
        example.pause();
        break;

    case GLFW_KEY_1:
        example.setProcessing(Particles::ProcessingMode::CPU);
        std::cout << "Processing: CPU" << std::endl;
        break;
    case GLFW_KEY_2:
        example.setProcessing(Particles::ProcessingMode::CPU_OMP);
        std::cout << "Processing: CPU_OMP" << std::endl;
        break;
    case GLFW_KEY_3:
        example.setProcessing(Particles::ProcessingMode::CPU_OMP_SSE41);
        std::cout << "Processing: CPU_OMP_SSE41" << std::endl;
        break;
    case GLFW_KEY_4:
        example.setProcessing(Particles::ProcessingMode::CPU_OMP_AVX2);
        std::cout << "Processing: CPU_OMP_AVX2" << std::endl;
        break;
    case GLFW_KEY_5:
        example.setProcessing(Particles::ProcessingMode::GPU_ComputeShaders);
        std::cout << "Processing: GPU_ComputeShaders" << std::endl;
#ifdef __APPLE__
        std::cout << "not supported by OS X" << std::endl;
#endif
        break;
    case GLFW_KEY_6:
        example.setDrawing(Particles::DrawingMode::None);
        std::cout << "Drawing: None" << std::endl;
        break;
    case GLFW_KEY_7:
        example.setDrawing(Particles::DrawingMode::BuiltInPoints);
        std::cout << "Drawing: Points" << std::endl;
        break;
    case GLFW_KEY_8:
        example.setDrawing(Particles::DrawingMode::CustomQuads);
        std::cout << "Drawing: Quads" << std::endl;
        break;
    case GLFW_KEY_9:
        example.setDrawing(Particles::DrawingMode::ShadedQuads);
        std::cout << "Drawing: ShadedQuads" << std::endl;
        break;    
    case GLFW_KEY_0:
        example.setDrawing(Particles::DrawingMode::Fluid);
        std::cout << "Drawing: Fluid" << std::endl;
        break;
    }
}


// "In case a GLFW function fails, an error is reported to the GLFW 
// error callback. You can receive these reports with an error
// callback." http://www.glfw.org/docs/latest/quick.html#quick_capture_error
void errorCallback(int errnum, const char * errmsg)
{
    std::cerr << errnum << ": " << errmsg << std::endl;
}


}


int main(int /*argc*/, char ** /*argv*/)
{
    if (!glfwInit())
    {
        return 1;
    }

    glfwSetErrorCallback(errorCallback);

    glfwDefaultWindowHints();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow * window = glfwCreateWindow(canvasWidth, canvasHeight, "", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return 2;
    }

    glfwSetFramebufferSizeCallback(window, resizeCallback);
    glfwSetKeyCallback(window, keyCallback);

    std::cout << "Particles (CPU/GPU)" << std::endl << std::endl;

    std::cout 
        << "  [F5] reload shaders" << std::endl
        << "  [F6] benchmark" << std::endl
        << "  [Space] pause processing (toggle)" << std::endl
        << std::endl
        << "  [1] particle processing: CPU default" << std::endl
        << "  [2] particle processing: CPU_OMP" << std::endl
        << "  [3] particle processing: CPU_OMP_SSE41" << std::endl
        << "  [4] particle processing: CPU_OMP_AVX2" << std::endl
        << "  [5] particle processing: GPU_ComputeShaders" << std::endl
        << std::endl
        << "  [6] particle drawing: none/skip" << std::endl
        << "  [7] particle drawing: built-in points" << std::endl
        << "  [8] particle drawing: custom quads" << std::endl
        << "  [9] particle drawing: custom, shaded quads" << std::endl
        << "  [0] particle drawing: fluid" << std::endl
        << std::endl
        << "  [a/d] rotate left/right" << std::endl
        << "  [S/s] increase/decrease particle scale" << std::endl
        << std::endl;

    glfwMakeContextCurrent(window);

    glbinding::Binding::initialize(false);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    example.resize(width, height);
    example.initialize();

    while (!glfwWindowShouldClose(window)) // main loop
    {
        glfwPollEvents();

        example.render();

        glfwSwapBuffers(window);
    }

    example.cleanup();

    glfwMakeContextCurrent(nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}
