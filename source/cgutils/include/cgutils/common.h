
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

#pragma warning(push)
#pragma warning(disable : 4201)
#include <glm/vec2.hpp>
#pragma warning(pop)

#include <glbinding/gl/types.h>

#include <cgutils/cgutils_api.h>

namespace cgutils
{

// This function is reads the contents of the back left buffer and 
// stores it into a PPM formated image file http://netpbm.sourceforge.net/doc/ppm.html
CGUTILS_API void captureAsPPM(const char * filePath, unsigned int width, unsigned int height);

// Read raw binary file into a char vector (probably the fastest way).
CGUTILS_API std::vector<char> rawFromFile(const char * filePath);

CGUTILS_API std::string textFromFile(const char * filePath);

CGUTILS_API bool createShader(gl::GLenum type, const std::string & name, const std::string & source, gl::GLuint & id);
CGUTILS_API bool createProgram(const std::string & name, gl::GLuint vertexShader, gl::GLuint fragmentShader, gl::GLuint & id);

CGUTILS_API bool checkForCompilationError(gl::GLuint shader, const std::string & identifier);
CGUTILS_API bool checkForLinkerError(gl::GLuint program, const std::string & identifier);

CGUTILS_API float  mse(const std::vector<glm::vec4> & I, const std::vector<glm::vec4> & K, const float scale);
CGUTILS_API float psnr(const std::vector<glm::vec4> & I, const std::vector<glm::vec4> & K, const float scale);

CGUTILS_API std::string humanFileSize(const size_t bytes, const int decimals = 2);
CGUTILS_API std::string humanTimeDuration(const std::uint64_t nanoseconds, const int decimals = 2);

} // namespace cgutils
