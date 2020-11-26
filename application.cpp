#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <thread>
#include <mutex>
std::mutex TOI_mutex;

#include <windows.h>
#include <vector>


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"

#include "hMapGen.h"



struct ShaderSources
{
    std::string Vertexsource;
    std::string FragmentSource;
    std::string ControlSource;
    std::string EvaluationSource;
};

struct Objbuffers
{
    unsigned int vbo;
    unsigned int ibo;
    unsigned int vao;
    unsigned int icount;
    unsigned int text_id;
    unsigned int terrain_rock_texture_id;
    unsigned int terrain_rock_normal_id;
    unsigned int normal_id;
    unsigned int normal2_id;
    unsigned int displacement_id;
};

struct FBO
{
    unsigned int id;
    unsigned int text_id;
    unsigned int rbo;
    unsigned int depth;

};

struct GBUFFER
{
    unsigned int gBuffer;
    unsigned int gPosition;
    unsigned int gNormal;
    unsigned int gColor;
    unsigned int gDepth;
};

static unsigned int CompileShader(unsigned int type, const std::string& source)
{
    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    //errors?
    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE)
    {
        int len;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &len);
        //noniin.. tervettuloo c++.. kaytannossa sama ku char msg[len]; allokoi stackista tilaa tolle error messagelle
        char* message = (char*)alloca(len * sizeof(char));
        glGetShaderInfoLog(id, len, &len, message);
        std::cout << message << std::endl;
    }

    return id;
}

static unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader)
{
    unsigned int program = glCreateProgram();
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);

    //naa on angetty tohon ohjelmaan nii ei enaa tarvita
    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

static unsigned int CreateTesselatedShader(const std::string& vertexShader, const std::string& fragmentShader, const std::string& TesselationControlShader, const std::string& TesselationEvaluationShader)
{
    unsigned int program = glCreateProgram();
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int tcs = CompileShader(GL_TESS_CONTROL_SHADER, TesselationControlShader);
    unsigned int tes = CompileShader(GL_TESS_EVALUATION_SHADER, TesselationEvaluationShader);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

    glAttachShader(program, vs);
    glAttachShader(program, tcs);
    glAttachShader(program, tes);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);

    //naa on angetty tohon ohjelmaan nii ei enaa tarvita
    glDeleteShader(vs);
    glDeleteShader(tcs);
    glDeleteShader(tes);
    glDeleteShader(fs);

    return program;
}

static ShaderSources Parseshader(const std::string& file, bool tesselation)
{
    std::ifstream stream(file);

    enum class ShaderType
    {
        NONE = -1, VERTEX = 0, FRAGMENT = 1, TCONTROL = 2, TEVALUATION = 3
    };

    std::string line;
    std::stringstream ss[4];
    ShaderType type = ShaderType::NONE;
    while (getline(stream, line))
    {
        //std::cout << line << std::endl;
        if (line.find("#shader") != std::string::npos)
        {
            if (line.find("vertex") != std::string::npos)
            {
                type = ShaderType::VERTEX;
            }
            else if (line.find("fragment") != std::string::npos)
            {
                type = ShaderType::FRAGMENT;
            }
            else if (line.find("tcs") != std::string::npos)
            {
                type = ShaderType::TCONTROL;
            }
            else if (line.find("tes") != std::string::npos)
            {
                type = ShaderType::TEVALUATION;
            }
        }
        else
        {
            ss[(int)type] << line << "\n";
        }
    }
    if (tesselation)
    {
        return { ss[0].str(), ss[1].str(), ss[2].str(), ss[3].str() };
    }
    else
    {
        return { ss[0].str(), ss[1].str() };
    }
}

void GetDesktopResolution(int& horizontal, int& vertical)
{
    RECT desktop;
    // Get a handle to the desktop window
    const HWND hDesktop = GetDesktopWindow();
    // Get the size of screen to the variable desktop
    GetWindowRect(hDesktop, &desktop);
    // The top left corner will have coordinates (0,0)
    // and the bottom right corner will have coordinates
    // (horizontal, vertical)
    horizontal = desktop.right;
    vertical = desktop.bottom;
}

GLFWwindow* init_windowJaOpenGL()
{
    GLFWwindow* window;

    /* Initialize the library */
    glfwInit();

    //glfwWindowHint(GLFW_RESIZABLE, false);
    //glfwWindowHint(GLFW_DECORATED, false);
    //glfwWindowHint(GLFW_DOUBLEBUFFER,true);
    glfwWindowHint(GLFW_SAMPLES, 4);
    //glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, true);

    int horizontal = 0;
    int vertical = 0;

    GetDesktopResolution(horizontal, vertical);

    /* Create a windowed mode window and its OpenGL context */
    //window = glfwCreateWindow(horizontal-1, vertical-1, "juuh elikkas", NULL, NULL);
    window = glfwCreateWindow(1600, 600, "juuh elikkas", NULL, NULL);


    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    //tan pitaa olla contextin jalq
    if (glewInit() != GLEW_OK)
    {
        std::cout << "no eipa ollu kunnossa\n";
    }
    ImGui::CreateContext();

    return window;
}

Objbuffers Createobject(std::vector<float> veebeeoo)
{

    //float* arr = &vert[0];
    unsigned int vectorin_koko = sizeof(float) * veebeeoo.size();


    /*float vertices[] = {
        -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
    };*/

    
    unsigned int indices[] = {
        0, 1 , 2
    };

    //unsigned int icount = sizeof(indices)/4;
    //std::cout << sizeof(vertices) << std::endl;

    unsigned int icount = veebeeoo.size() / 8;

    unsigned int vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    //bufferin generointi indexilla
    unsigned int vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    //datat bufferiin bytekoko ite data ja kayttotarkotus
    glBufferData(GL_ARRAY_BUFFER, vectorin_koko, &veebeeoo[0], GL_STATIC_DRAW);


    unsigned int ibo;
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    //datat bufferiin bytekoko ite data ja kayttotarkotus
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (const void*)(sizeof(float) * 3));
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (const void*)(sizeof(float) * 5));
    //etta naa pointterit voi antaa pitaa olla bindattuna molemmat arr buf ja ele arr buf tarkennus.. jos siis kaytta draw elee

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return { vbo, ibo, vao, icount };
}

void SetUniMat4(unsigned int shader, glm::mat4 matriisi, const GLchar* name)
{
    int modelLoc = glGetUniformLocation(shader, name);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(matriisi));
}

void SetUniVec3(unsigned int shader, glm::vec3 vektori, const GLchar* name)
{
    glUniform3fv(glGetUniformLocation(shader, name), 1, glm::value_ptr(vektori));
}

unsigned int make_shader(std::string path, bool tesselation)
{
    unsigned int shader;
    ShaderSources source = Parseshader(path, tesselation);
    if (tesselation)
    {
        shader = CreateTesselatedShader(source.Vertexsource, source.FragmentSource, source.ControlSource, source.EvaluationSource);
    }
    else
    {
        shader = CreateShader(source.Vertexsource, source.FragmentSource);
    }
    return shader;
}

void konsolipiiloon()
{
    ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
}

void konsolitakas()
{
    ::ShowWindow(::GetConsoleWindow(), SW_SHOW);
}

struct ImGuiData
{
    float yrot;
    float xrot;
    float zrot;
};

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;

    while (getline(ss, item, delim)) {
        result.push_back(item);
    }

    return result;
}

std::vector<float> ParseOBJ(const std::string& file)
{
    std::ifstream stream(file);
    std::string line;
    std::vector<std::string> str_vec;
    std::vector<std::string> str_vec_sub;

    std::vector<std::vector<float>> v;
    std::vector<std::vector<float>> vt;
    std::vector<std::vector<float>> vn;

    std::vector<float> vbo;

    while (getline(stream, line))
    {
        std::vector<float> sub;

        if (line[0] == 'v' && line[1] == ' ')
        {
            str_vec = split(line, ' ');
            for (auto i : str_vec)
            {
                if (i != "v" && i != "")
                {
                    sub.emplace_back(std::stof(i));
                }
            }
            v.emplace_back(sub);
        }
        else if (line[0] == 'v' && line[1] == 't')
        {
            str_vec = split(line, ' ');
            for (auto i : str_vec)
            {
                if (i != "vt" && i != "")
                {
                    sub.emplace_back(std::stof(i));
                }
            }
            vt.emplace_back(sub);
        }
        else if (line[0] == 'v' && line[1] == 'n')
        {
            str_vec = split(line, ' ');
            for (auto i : str_vec)
            {
                if (i != "vn" && i != "")
                {
                    sub.emplace_back(std::stof(i));
                }
            }
            vn.emplace_back(sub);
        }
        else if (line[0] == 'f' && line[1] == ' ')
        {
            str_vec = split(line, ' ');
            for (auto i : str_vec)
            {
                if (i != "f" && i != "")
                {
                    str_vec_sub = split(i, '/');
                    int counter = 0;
                    for (auto j : str_vec_sub)
                    {
                        switch (counter)
                        {
                        case 0:
                            //masteriin vsta
                            for (auto x : v[(std::stoi(j))-1])
                            {
                                vbo.emplace_back(x);
                            }
                            break;
                        case 1:
                            //masteriin vtsta
                            for (auto x : vt[(std::stoi(j)) - 1])
                            {
                                vbo.emplace_back(x);
                            }
                            break;
                        case 2:
                            //masteriin vnta
                            for (auto x : vn[(std::stoi(j)) - 1])
                            {
                                vbo.emplace_back(x);
                            }
                            break;
                        default:
                            break;
                        }
                        counter++;
                    }
                }
            }
        }
    }
    return vbo;
}

class KAMERA
{
private:
    glm::mat4 view = glm::mat4(1.0f);
public:
    glm::vec2 camera_LAST_TILE = glm::vec2(0, 0);
    glm::vec2 cameraTILE = glm::vec2(0, 0);
    glm::vec3 cameraPos = glm::vec3(0.0f, 100.0f, 0.0f);
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    short muutostileIndex = 0;
    bool firstMouse = true;
    float lastX = 400;
    float lastY = 300;
    float pitch = 0;
    float yaw = -90;
    float sensitivity;
    float cameraSpeed = 0.5f;
    float speed1 = 0.5f;
    float speed2 = 10.0f;
    bool enabled = true;


    glm::mat4 getview()
    {
        view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        return view;
    }
};

KAMERA kamera;


class TERRAIN_OBJEKTI
{
private:
    Objbuffers buffers;
    glm::mat4 model;
    glm::mat4 TI_model = glm::mat4(1.0f);
    glm::mat4 rot_mod = glm::mat4(1.0f);
    glm::mat4 trans_mod = glm::mat4(1.0f);
    glm::mat4 scale_mod = glm::mat4(1.0f);
    bool texture_loaded = false;
    bool normal_loaded = false;
    bool displacement_loaded = false;


    unsigned short* buf1;
    unsigned char* buf2;

    std::vector<glm::mat4> laatikko_insta_model_next; //tarvii createfunktioon jonkun nextpos homman

public:
    glm::vec3 position = glm::vec3(0, 0, 0);
    glm::mat4 getmodel()
    {
        model = trans_mod * rot_mod * scale_mod;
        return model;
    }
    bool neednewmaps = true;


    std::vector<int> read_heights_for_insta_at_start(const std::string& path)
    {
        unsigned short* localbuffer;
        int width, height, BPP;
        stbi_set_flip_vertically_on_load(1);
        localbuffer = stbi_load_16(path.c_str(), &width, &height, &BPP, 1);
        std::vector<int> hait(width*height);
        /*
        for (int i = 0; i < 100000; i++)
        {
            std::cout << (int)localbuffer[i] << std::endl;
        }*/
        return hait;

    }
    std::vector<glm::mat4> generate_laatikko_insta_models()
    {
        float randx = 0;
        float randz = 0;
        float height = 0;
        int scalerandx = 0;
        int scalerandz = 0;

        std::vector<glm::mat4> laatikko_insta_model;
        for (int i = 0; i < 1000; i++)
        {
            randx = (float)(rand() % 3999 + 1) - 2000.0;
            randz = (float)(rand() % 3999 + 1) - 2000.0;
            scalerandx = (int)(0.0 + (1024.0 - 0.0) * (((float)randx - (float)-2000.0) / ((float)2000 - (float)-2000.0)));
            scalerandz = (int)(0.0 + (1024.0 - 0.0) * (((float)randz - (float)-2000.0) / ((float)2000 - (float)-2000.0)));
            //std::cout << scalerandx << " " << scalerandz << std::endl;

            //std::cout << scalerandx <<"," << scalerandz << std::endl;

            //scalerandx = (roundf((float)scalerandx * cos(1.57079633) - (float)scalerandz * sin(1.57079633))+1024);
            //std::cout << scalerandx << std::endl;
            //scalerandz = (roundf((float)scalerandx * sin(1.57079633) + (float)scalerandz * cos(1.57079633)));
            //std::cout << scalerandz << std::endl;

            height = (0.0 + (700.0 - 0.0) * (((float)(this->buf1[scalerandz * 1024 + scalerandx]) - 0.0) / (65536.0 - 0.0)));

            //std::cout << (int)height << std::endl;
            laatikko_insta_model.emplace_back(glm::translate(glm::mat4(1.0f), glm::vec3(position[0] + randx, height, position[2] + randz)));
        }
        return laatikko_insta_model;
    }

    void load_model(const std::string& file)
    {
        std::vector<float> vbo = ParseOBJ(file);
        buffers = Createobject(vbo);
    }
    void load_textures(const std::string& path)
    {
        unsigned char* localbuffer;
        int width, height, BPP;

        stbi_set_flip_vertically_on_load(1);
        localbuffer = stbi_load(path.c_str(), &width, &height, &BPP, 4);
        glGenTextures(1, &buffers.text_id);
        glBindTexture(GL_TEXTURE_2D, buffers.text_id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, GL_MAX_TEXTURE_MAX_ANISOTROPY);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, localbuffer);
        glBindTexture(GL_TEXTURE_2D, 0);

        if (localbuffer)
        {
            stbi_image_free(localbuffer);
        }
        texture_loaded = true;
    }
    void load_terrain_rock_text(const std::string& path)
    {
        unsigned char* localbuffer;
        int width, height, BPP;

        stbi_set_flip_vertically_on_load(1);
        localbuffer = stbi_load(path.c_str(), &width, &height, &BPP, 4);
        glGenTextures(1, &buffers.terrain_rock_texture_id);
        glBindTexture(GL_TEXTURE_2D, buffers.terrain_rock_texture_id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, GL_MAX_TEXTURE_MAX_ANISOTROPY);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, localbuffer);
        glBindTexture(GL_TEXTURE_2D, 0);

        if (localbuffer)
        {
            stbi_image_free(localbuffer);
        }
        texture_loaded = true;
    }
    void load_normals(const std::string& path)
    {
        unsigned char* localbuffer;
        int width, height, BPP;

        stbi_set_flip_vertically_on_load(1);
        localbuffer = stbi_load(path.c_str(), &width, &height, &BPP, 4);

        glGenTextures(1, &buffers.normal_id);
        glBindTexture(GL_TEXTURE_2D, buffers.normal_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, localbuffer);
        glBindTexture(GL_TEXTURE_2D, 0);

        if (localbuffer)
        {
            stbi_image_free(localbuffer);
        }
        normal_loaded = true;
    }
    void load_normals2(const std::string& path)
    {
        unsigned char* localbuffer;
        int width, height, BPP;

        stbi_set_flip_vertically_on_load(1);
        localbuffer = stbi_load(path.c_str(), &width, &height, &BPP, 4);

        glGenTextures(1, &buffers.normal2_id);
        glBindTexture(GL_TEXTURE_2D, buffers.normal2_id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, GL_MAX_TEXTURE_MAX_ANISOTROPY);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, localbuffer);
        glBindTexture(GL_TEXTURE_2D, 0);

        if (localbuffer)
        {
            stbi_image_free(localbuffer);
        }
        normal_loaded = true;
    }
    void load_ter_rock_normal(const std::string& path)
    {
        unsigned char* localbuffer;
        int width, height, BPP;

        stbi_set_flip_vertically_on_load(1);
        localbuffer = stbi_load(path.c_str(), &width, &height, &BPP, 4);

        glGenTextures(1, &buffers.terrain_rock_normal_id);
        glBindTexture(GL_TEXTURE_2D, buffers.terrain_rock_normal_id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, GL_MAX_TEXTURE_MAX_ANISOTROPY);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, localbuffer);
        glBindTexture(GL_TEXTURE_2D, 0);

        if (localbuffer)
        {
            stbi_image_free(localbuffer);
        }
    }
    void load_displacement(const std::string& path)
    {
        unsigned short* localbuffer;
        int width, height, BPP;

        //stbi_set_flip_vertically_on_load(1);
        localbuffer = stbi_load_16(path.c_str(), &width, &height, &BPP, 1);

        glGenTextures(1, &buffers.displacement_id);
        glBindTexture(GL_TEXTURE_2D, buffers.displacement_id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, width, height, 0, GL_RED, GL_UNSIGNED_SHORT, localbuffer);
        glBindTexture(GL_TEXTURE_2D, 0);

        if (localbuffer)
        {
            stbi_image_free(localbuffer);
        }
        displacement_loaded = true;
    }
    void load_displacement_to_buf1(const std::string& path)
    {
        int width, height, BPP;
        //stbi_set_flip_vertically_on_load(1);
        //juu pitaa clearaa tassa ennen ku ankee uutta soosia tulille
        if (this->buf1)
        {
            stbi_image_free(this->buf1);
        }
        this->buf1 = stbi_load_16(path.c_str(), &width, &height, &BPP, 1);
    }
    void load_displacement_binaries_to_buf1(unsigned short* bins)
    {
        this->buf1 = bins;
    }
    void load_blendmap_to_buf2(const std::string& path)
    {
        unsigned char* localbuffer;
        int width, height, BPP;
        stbi_set_flip_vertically_on_load(1);
        localbuffer = stbi_load(path.c_str(), &width, &height, &BPP, 4);
        this->buf2 = localbuffer;
    }
    void load_from_buf1_to_GPU()
    {
        glGenTextures(1, &buffers.displacement_id);
        glBindTexture(GL_TEXTURE_2D, buffers.displacement_id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, 1024, 1024, 0, GL_RED, GL_UNSIGNED_SHORT, this->buf1);
        glBindTexture(GL_TEXTURE_2D, 0);

        displacement_loaded = true;
    }
    void load_from_buf2_to_GPU()
    {
        glGenTextures(1, &buffers.text_id);
        glBindTexture(GL_TEXTURE_2D, buffers.text_id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, GL_MAX_TEXTURE_MAX_ANISOTROPY);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, this->buf2);
        glBindTexture(GL_TEXTURE_2D, 0);

        displacement_loaded = true;
    }
    void print()
    {
        std::cout << "icount: " << buffers.icount << std::endl;
        std::cout << "ibo: " << buffers.ibo << std::endl;
        std::cout << "vao: " << buffers.vao << std::endl;
        std::cout << "vbo: " << buffers.vbo << std::endl;
    }
    void send_model_uniforms(unsigned int shader)
    {
        model = trans_mod * rot_mod * scale_mod;
        SetUniMat4(shader, model, "model");
    }
    void send_TI_model_uniforms(unsigned int shader)
    {
        TI_model = glm::transpose(glm::inverse(model));
        SetUniMat4(shader, TI_model, "TI_model");
    }
    void draw_obj(unsigned int shader, unsigned int FBO_tekstuuri = 1)
    {
        glBindVertexArray(buffers.vao);

        glActiveTexture(GL_TEXTURE0);

        if (texture_loaded && FBO_tekstuuri == 1)
        {
            glBindTexture(GL_TEXTURE_2D, buffers.text_id);
        }
        else if (!texture_loaded && FBO_tekstuuri != 1)
        {
            glBindTexture(GL_TEXTURE_2D, FBO_tekstuuri);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, 1);
        }


        glUniform1i(glGetUniformLocation(shader, "tex"), 0);

        //glDrawElements(GL_TRIANGLES, buffers.icount, GL_UNSIGNED_INT, nullptr);
        //glDrawArrays(GL_LINES, 0, 4);
        glDrawArrays(GL_TRIANGLES, 0, buffers.icount);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    void draw_tesselated_obj(unsigned int shader, unsigned int FBO_tekstuuri = 1)
    {
        glBindVertexArray(buffers.vao);

        glActiveTexture(GL_TEXTURE0);

        if (texture_loaded)
        {
            glBindTexture(GL_TEXTURE_2D, buffers.text_id);
        }
        else if (!texture_loaded && FBO_tekstuuri != 1)
        {
            glBindTexture(GL_TEXTURE_2D, FBO_tekstuuri);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, 1);
        }

        glUniform1i(glGetUniformLocation(shader, "tex"), 0);

        if (displacement_loaded)
        {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, buffers.displacement_id);
            glUniform1i(glGetUniformLocation(shader, "gDisplacementMap"), 1);
        }
        if (normal_loaded)
        {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, buffers.normal_id);
            glUniform1i(glGetUniformLocation(shader, "normalMap"), 2);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, buffers.normal2_id);
            glUniform1i(glGetUniformLocation(shader, "normalMap2"), 3);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, buffers.terrain_rock_texture_id);
            glUniform1i(glGetUniformLocation(shader, "rocktext"), 4);
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, buffers.terrain_rock_normal_id);
            glUniform1i(glGetUniformLocation(shader, "rocknormal"), 5);

        }

        glDrawArrays(GL_PATCHES, 0, buffers.icount);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    void rotate_obj(float radx, float rady, float radz)
    {
        rot_mod = glm::mat4(1.0f);
        rot_mod = glm::rotate(rot_mod, glm::radians(radx), glm::vec3(1.0f, 0.0f, 0.0f));
        rot_mod = glm::rotate(rot_mod, glm::radians(rady), glm::vec3(0.0f, 1.0f, 0.0f));
        rot_mod = glm::rotate(rot_mod, glm::radians(radz), glm::vec3(0.0f, 0.0f, 1.0f));
    }
    void trans_obj(glm::vec3 v)
    {
        trans_mod = glm::translate(glm::mat4(1.0f), v);
    }
    void scale_obj(float scaler)
    {
        scale_mod = glm::scale(glm::mat4(1.0f), glm::vec3(scaler, scaler, scaler));
    }
    void printbuf()
    {
        for (int i = 0; i < 100000; i+=4)
        {
            std::cout << (int)this->buf1[i] << std::endl;
        }
    }
};

class OBJEKTI
{
private:
    Objbuffers buffers;
    glm::mat4 model;
    glm::mat4 TI_model = glm::mat4(1.0f);
    glm::mat4 rot_mod = glm::mat4(1.0f);
    glm::mat4 trans_mod = glm::mat4(1.0f);
    glm::mat4 scale_mod = glm::mat4(1.0f);
    bool texture_loaded = false;
    bool normal_loaded = false;
    bool displacement_loaded = false;
    unsigned int instanssimaara = 0;
public:
    glm::mat4 getmodel()
    {
        model = trans_mod * rot_mod * scale_mod;
        return model;
    }

    void bind_instance_model_matrixes(std::vector<glm::mat4> models)
    {
        this->instanssimaara = models.size();
        unsigned int listakoko = this->instanssimaara * sizeof(glm::mat4);

        unsigned int buffer;
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, listakoko, &models[0], GL_STATIC_DRAW);

        glBindVertexArray(buffers.vao);
        std::size_t vec4Size = sizeof(glm::vec4);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(1 * vec4Size));
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));

        glVertexAttribDivisor(3, 1);
        glVertexAttribDivisor(4, 1);
        glVertexAttribDivisor(5, 1);
        glVertexAttribDivisor(6, 1);

        glBindVertexArray(0);
    }

    void load_model(const std::string& file)
    {
        std::vector<float> vbo = ParseOBJ(file);
        buffers = Createobject(vbo);
    }
    void load_textures(const std::string& path)
    {
        unsigned char* localbuffer;
        int width, height, BPP;

        stbi_set_flip_vertically_on_load(1);
        localbuffer = stbi_load(path.c_str(),&width,&height,&BPP,4);
        glGenTextures(1, &buffers.text_id);
        glBindTexture(GL_TEXTURE_2D, buffers.text_id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, GL_MAX_TEXTURE_MAX_ANISOTROPY);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
        glTexImage2D(GL_TEXTURE_2D, 0 ,GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, localbuffer);
        glBindTexture(GL_TEXTURE_2D, 0);

        if (localbuffer)
        {
            stbi_image_free(localbuffer);
        }
        texture_loaded = true;
    }
    void load_normals(const std::string& path)
    {
        unsigned char* localbuffer;
        int width, height, BPP;

        stbi_set_flip_vertically_on_load(1);
        localbuffer = stbi_load(path.c_str(), &width, &height, &BPP, 4);

        glGenTextures(1, &buffers.normal_id);
        glBindTexture(GL_TEXTURE_2D, buffers.normal_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, localbuffer);
        glBindTexture(GL_TEXTURE_2D, 0);

        if (localbuffer)
        {
            stbi_image_free(localbuffer);
        }
        normal_loaded = true;
    }
    void load_normals2(const std::string& path)
    {
        unsigned char* localbuffer;
        int width, height, BPP;

        stbi_set_flip_vertically_on_load(1);
        localbuffer = stbi_load(path.c_str(), &width, &height, &BPP, 4);

        glGenTextures(1, &buffers.normal2_id);
        glBindTexture(GL_TEXTURE_2D, buffers.normal2_id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, GL_MAX_TEXTURE_MAX_ANISOTROPY);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, localbuffer);
        glBindTexture(GL_TEXTURE_2D, 0);

        if (localbuffer)
        {
            stbi_image_free(localbuffer);
        }
        normal_loaded = true;
    }
    void load_displacement(const std::string& path)
    {
        unsigned short* localbuffer;
        int width, height, BPP;

        stbi_set_flip_vertically_on_load(1);
        localbuffer = stbi_load_16(path.c_str(), &width, &height, &BPP, 4);

        glGenTextures(1, &buffers.displacement_id);
        glBindTexture(GL_TEXTURE_2D, buffers.displacement_id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, width, height, 0, GL_RGBA, GL_UNSIGNED_SHORT, localbuffer);
        glBindTexture(GL_TEXTURE_2D, 0);

        if (localbuffer)
        {
            stbi_image_free(localbuffer);
        }
        displacement_loaded = true;
    }
    void print()
    {
        std::cout << "icount: " << buffers.icount << std::endl;
        std::cout <<"ibo: " <<buffers.ibo<< std::endl;
        std::cout <<"vao: " <<buffers.vao << std::endl;
        std::cout << "vbo: " <<buffers.vbo << std::endl;
    }
    void send_model_uniforms(unsigned int shader)
    {
        model = trans_mod * rot_mod * scale_mod;
        SetUniMat4(shader, model, "model");
    }
    void send_TI_model_uniforms(unsigned int shader)
    {
        TI_model = glm::transpose(glm::inverse(model));
        SetUniMat4(shader, TI_model, "TI_model");
    }
    void draw_obj(unsigned int shader,unsigned int FBO_tekstuuri = 1)
    {
        glBindVertexArray(buffers.vao);

        glActiveTexture(GL_TEXTURE0);

        if (texture_loaded && FBO_tekstuuri == 1)
        {
            glBindTexture(GL_TEXTURE_2D, buffers.text_id);
        }
        else if (!texture_loaded && FBO_tekstuuri != 1)
        {
            glBindTexture(GL_TEXTURE_2D, FBO_tekstuuri);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, 1);
        }


        glUniform1i(glGetUniformLocation(shader, "tex"), 0);

        //glDrawElements(GL_TRIANGLES, buffers.icount, GL_UNSIGNED_INT, nullptr);
        //glDrawArrays(GL_LINES, 0, 4);
        glDrawArrays(GL_TRIANGLES, 0, buffers.icount);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    void draw_lightpass(unsigned int shader, unsigned int pos, unsigned int normal, unsigned int color, glm::vec3 campos, glm::mat4 projection, glm::mat4 lightProjection, glm::mat4 lightView,FBO shadows)
    {
        glUseProgram(shader);
        glBindVertexArray(buffers.vao);

        SetUniVec3(shader, campos, "gEyeWorldPos");
        SetUniMat4(shader, lightView, "lightView");
        SetUniMat4(shader, lightProjection, "lightProjection");
        SetUniMat4(shader, kamera.getview(), "cameraView");

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pos);
        glUniform1i(glGetUniformLocation(shader, "Pos"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normal);
        glUniform1i(glGetUniformLocation(shader, "Norm"), 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, color);
        glUniform1i(glGetUniformLocation(shader, "Color"), 2);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, shadows.depth);
        glUniform1i(glGetUniformLocation(shader, "shadowmap"), 3);


        glDrawArrays(GL_TRIANGLES, 0, buffers.icount);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
    }
    void draw_tesselated_obj(unsigned int shader, unsigned int FBO_tekstuuri = 1)
    {
        glBindVertexArray(buffers.vao);

        glActiveTexture(GL_TEXTURE0);

        if (texture_loaded)
        {
            glBindTexture(GL_TEXTURE_2D, buffers.text_id);
        }
        else if (!texture_loaded && FBO_tekstuuri != 1)
        {
            glBindTexture(GL_TEXTURE_2D, FBO_tekstuuri);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, 1);
        }

        glUniform1i(glGetUniformLocation(shader, "tex"), 0);

        if (displacement_loaded)
        {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, buffers.displacement_id);
            glUniform1i(glGetUniformLocation(shader, "gDisplacementMap"), 1);
        }
        if(normal_loaded)
        {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, buffers.normal2_id);
            glUniform1i(glGetUniformLocation(shader, "normalMap"), 2);
        }

        glDrawArrays(GL_PATCHES, 0, buffers.icount);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    void draw_obj_instanced(unsigned int shader, unsigned int FBO_tekstuuri = 1)
    {
        glBindVertexArray(buffers.vao);

        glActiveTexture(GL_TEXTURE0);

        if (texture_loaded && FBO_tekstuuri == 1)
        {
            glBindTexture(GL_TEXTURE_2D, buffers.text_id);
        }
        else if (!texture_loaded && FBO_tekstuuri != 1)
        {
            glBindTexture(GL_TEXTURE_2D, FBO_tekstuuri);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, 1);
        }

        if (normal_loaded)
        {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, buffers.normal2_id);
            glUniform1i(glGetUniformLocation(shader, "normalMap"), 1);
        }


        glUniform1i(glGetUniformLocation(shader, "tex"), 0);

        glDrawArraysInstanced(GL_TRIANGLES, 0, buffers.icount, this->instanssimaara);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    void draw_tesselated_instanced_obj(unsigned int shader, unsigned int FBO_tekstuuri = 1)
    {
        glBindVertexArray(buffers.vao);

        glActiveTexture(GL_TEXTURE0);

        if (texture_loaded)
        {
            glBindTexture(GL_TEXTURE_2D, buffers.text_id);
        }
        else if (!texture_loaded && FBO_tekstuuri != 1)
        {
            glBindTexture(GL_TEXTURE_2D, FBO_tekstuuri);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, 1);
        }

        glUniform1i(glGetUniformLocation(shader, "tex"), 0);

        if (displacement_loaded)
        {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, buffers.displacement_id);
            glUniform1i(glGetUniformLocation(shader, "gDisplacementMap"), 1);
        }
        if (normal_loaded)
        {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, buffers.normal2_id);
            glUniform1i(glGetUniformLocation(shader, "normalMap"), 2);
        }

        glDrawArraysInstanced(GL_PATCHES, 0, buffers.icount,this->instanssimaara);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    void rotate_obj(float radx, float rady, float radz)
    {
        rot_mod = glm::mat4(1.0f);
        rot_mod = glm::rotate(rot_mod, glm::radians(radx), glm::vec3(1.0f,0.0f,0.0f));
        rot_mod = glm::rotate(rot_mod, glm::radians(rady), glm::vec3(0.0f, 1.0f, 0.0f));
        rot_mod = glm::rotate(rot_mod, glm::radians(radz), glm::vec3(0.0f, 0.0f, 1.0f));
    }
    void trans_obj(glm::vec3 v)
    {
        trans_mod = glm::translate(glm::mat4(1.0f), v);
    }
    void scale_obj(float scaler)
    {
        scale_mod = glm::scale(glm::mat4(1.0f), glm::vec3(scaler, scaler, scaler));
    }
};


void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (kamera.firstMouse)
    {
        kamera.lastX = xpos;
        kamera.lastY = ypos;
        kamera.firstMouse = false;
    }

    float xoffset = xpos - kamera.lastX;
    float yoffset = kamera.lastY - ypos;
    kamera.lastX = xpos;
    kamera.lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    kamera.yaw += xoffset;
    kamera.pitch += yoffset;

    if (kamera.pitch > 89.0f)
        kamera.pitch = 89.0f;
    if (kamera.pitch < -89.0f)
        kamera.pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(kamera.yaw)) * cos(glm::radians(kamera.pitch));
    direction.y = sin(glm::radians(kamera.pitch));
    direction.z = sin(glm::radians(kamera.yaw)) * cos(glm::radians(kamera.pitch));
    kamera.cameraFront = glm::normalize(direction);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        kamera.cameraSpeed = kamera.speed2;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE)
        kamera.cameraSpeed = kamera.speed1;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        kamera.cameraPos += kamera.cameraSpeed * kamera.cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        kamera.cameraPos -= kamera.cameraSpeed * kamera.cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        kamera.cameraPos -= glm::normalize(glm::cross(kamera.cameraFront, kamera.cameraUp)) * kamera.cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        kamera.cameraPos += glm::normalize(glm::cross(kamera.cameraFront, kamera.cameraUp)) * kamera.cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        kamera.cameraPos += glm::normalize(glm::cross(kamera.cameraFront, glm::vec3(-1.0,0.0,0.0))) * kamera.cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
        kamera.cameraPos += glm::normalize(glm::cross(kamera.cameraFront, glm::vec3(1.0, 0.0, 0.0))) * kamera.cameraSpeed;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS)
    {
        if (kamera.enabled)
        {
            //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            mouse_callback(window, xpos, ypos);
        }
    }
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_RELEASE)
    {
        kamera.firstMouse = true;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        kamera.enabled = true;
    }
        
}

ImGuiData imgui_ruutu(GLFWwindow* window, ImGuiData floats)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("pfffft");
    ImGui::Text("juuh elikkas");
    
    if (ImGui::IsWindowHovered())
    {
        kamera.enabled = false;
    }


    if (ImGui::SliderFloat("rot x", &floats.xrot, 0.0f, 360.0f))
    {
        kamera.enabled = false;
    }
    if (ImGui::SliderFloat("rot y", &floats.yrot, 0.0f, 360.0f))
    {
        kamera.enabled = false;
    }
    if (ImGui::SliderFloat("rot z", &floats.zrot, 0.0f, 360.0f))
    {
        kamera.enabled = false;
    }

    ImGui::Text("%.1f FPS",ImGui::GetIO().Framerate);
    if (ImGui::Button("quit"))
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }


    ImGui::End();

    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return { floats.yrot, floats.xrot, floats.zrot };
}

FBO create_FBO(int windW, int windH, std::string type)
{
    unsigned int fbo;
    unsigned int texture;
    unsigned int rbo = 0;
    unsigned int depth;

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);


    if (type == "COLOR")
    {
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, windW, windH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    }
    else if (type == "DEPTH")
    {
        const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
        glGenTextures(1, &depth);
        glBindTexture(GL_TEXTURE_2D, depth);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F ,SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return { fbo, texture, rbo, depth };
}

GBUFFER create_Gbuffer(int windW, int windH)
{
    unsigned int gBuffer;
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    unsigned int gPosition, gNormal, gColor, gDepth;

    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, windW, windH, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, windW, windH, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

    glGenTextures(1, &gColor);
    glBindTexture(GL_TEXTURE_2D, gColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, windW, windH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gColor, 0);

    glGenTextures(1, &gDepth);
    glBindTexture(GL_TEXTURE_2D, gDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, windW, windH, 0, GL_DEPTH_COMPONENT, GL_FLOAT,NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gDepth, 0);

    // - tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);


    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return { gBuffer, gPosition, gNormal, gColor,gDepth };
}

void render_scene_simpshader(unsigned int shader, std::vector<OBJEKTI*> objektit, ImGuiData floats, glm::mat4 projection)
{
    glUseProgram(shader);
    SetUniMat4(shader, projection, "projection");
    SetUniMat4(shader, kamera.getview(), "view");

    //glm::vec3 Lpos = glm::vec3(0.0, 4.0, 0.0);
    //SetUniVec3(shader, Lpos, "lightpos");

    glm::vec3 ambient = glm::vec3(0.1, 0.1, 0.1);
    SetUniVec3(shader, ambient, "ambient");

    glDisable(GL_CULL_FACE);
    objektit[0]->rotate_obj(floats.xrot, floats.yrot, floats.zrot);
    objektit[0]->send_model_uniforms(shader);
    objektit[0]->send_TI_model_uniforms(shader);
    objektit[0]->draw_obj(shader);
    SetUniMat4(shader, glm::mat4(1.0f), "TI_model");

    //skyrot = (float)glfwGetTime()*5;
    //skybox.rotate_obj(skyrot, skyrot, skyrot); //rotate obj missatahansa nii kaikki kiarii
    
    ambient = glm::vec3(1.0f, 1.0f, 1.0f);
    SetUniVec3(shader, ambient, "ambient");
    
    objektit[1]->trans_obj(kamera.cameraPos);
    objektit[1]->send_model_uniforms(shader);
    objektit[1]->draw_obj(shader);
    
    objektit[2]->send_model_uniforms(shader);
    objektit[2]->draw_obj(shader);
    glEnable(GL_CULL_FACE);
    glUseProgram(0);
}

void render_scene_nolighting(unsigned int shader, std::vector<OBJEKTI*> objektit, ImGuiData floats, glm::mat4 projection)
{
    glUseProgram(shader);

    projection = projection * kamera.getview();

    objektit[0]->rotate_obj(floats.xrot, floats.yrot, floats.zrot);
    SetUniMat4(shader, projection * objektit[0]->getmodel(), "gworld");
    objektit[0]->draw_obj(shader);

    glDisable(GL_CULL_FACE);
    //SetUniMat4(shader, projection * objektit[1]->getmodel(), "gworld");
    //objektit[1]->draw_obj(shader);


    SetUniMat4(shader, projection * objektit[2]->getmodel(), "gworld");
    objektit[2]->draw_obj(shader);

    float skyrot = (float)glfwGetTime();
    objektit[1]->rotate_obj(0.0, skyrot, 0.0);
    SetUniMat4(shader, projection * objektit[1]->getmodel(), "gworld");
    objektit[1]->send_model_uniforms(shader);
    objektit[1]->draw_obj(shader);
    glEnable(GL_CULL_FACE);
    glUseProgram(0);
}

void render_terrain(unsigned int shader, std::vector<TERRAIN_OBJEKTI*> objektit, glm::mat4 projection, FBO FBO,std::vector<int*> TOI)
{
    glUseProgram(shader);

    SetUniVec3(shader, kamera.cameraPos, "gEyeWorldPos");
    SetUniMat4(shader, (projection * kamera.getview()) , "PV");

    kamera.cameraTILE[0] = (int)kamera.cameraPos[0] / 2000;
    kamera.cameraTILE[1] = (int)kamera.cameraPos[2] / 2000;

    if (kamera.cameraTILE != kamera.camera_LAST_TILE)
    {
        glm::vec2 nig = kamera.cameraTILE - kamera.camera_LAST_TILE;
        if (nig == glm::vec2(0, -1) && kamera.cameraTILE[1] >= 0)
        {
            if (abs((int)kamera.camera_LAST_TILE[1] % 2) == 1)
            {
                //std::cout << "GENDOWN" << std::endl;
                objektit[*TOI[0]]->position[2] -= 12000;
                objektit[*TOI[0]]->trans_obj(objektit[*TOI[0]]->position);
                objektit[*TOI[1]]->position[2] -= 12000;
                objektit[*TOI[1]]->trans_obj(objektit[*TOI[1]]->position);
                objektit[*TOI[2]]->position[2] -= 12000;
                objektit[*TOI[2]]->trans_obj(objektit[*TOI[2]]->position);
                //KOLMEEN YLLAOLEVAAN AINA LADATAA UUDET TEXTIT
                objektit[*TOI[0]]->load_from_buf2_to_GPU();
                objektit[*TOI[0]]->load_from_buf1_to_GPU();

                objektit[*TOI[1]]->load_from_buf2_to_GPU();
                objektit[*TOI[1]]->load_from_buf1_to_GPU();

                objektit[*TOI[2]]->load_from_buf2_to_GPU();
                objektit[*TOI[2]]->load_from_buf1_to_GPU();

                int temp1, temp2, temp3;
                temp1 = *TOI[6];
                temp2 = *TOI[7];
                temp3 = *TOI[8];
                *TOI[6] = *TOI[0];
                *TOI[7] = *TOI[1];
                *TOI[8] = *TOI[2];
                *TOI[0] = *TOI[3];
                *TOI[1] = *TOI[4];
                *TOI[2] = *TOI[5];
                *TOI[3] = temp1;
                *TOI[4] = temp2;
                *TOI[5] = temp3;

                objektit[*TOI[0]]->neednewmaps = true;
                objektit[*TOI[1]]->neednewmaps = true;
                objektit[*TOI[2]]->neednewmaps = true;

                //objektit[*TOI[6]]->neednewmaps = true;
                //objektit[*TOI[7]]->neednewmaps = true;
                //objektit[*TOI[8]]->neednewmaps = true;

            }
        }
        else if (nig == glm::vec2(0, -1) && kamera.cameraTILE[1] < 0)
        {
            if (abs((int)kamera.cameraTILE[1] % 2) == 1)
            {
                //std::cout << "GENDOWN" << std::endl;
                objektit[*TOI[0]]->position[2] -= 12000;
                objektit[*TOI[0]]->trans_obj(objektit[*TOI[0]]->position);
                objektit[*TOI[1]]->position[2] -= 12000;
                objektit[*TOI[1]]->trans_obj(objektit[*TOI[1]]->position);
                objektit[*TOI[2]]->position[2] -= 12000;
                objektit[*TOI[2]]->trans_obj(objektit[*TOI[2]]->position);

                objektit[*TOI[0]]->load_from_buf2_to_GPU();
                objektit[*TOI[0]]->load_from_buf1_to_GPU();
                objektit[*TOI[1]]->load_from_buf2_to_GPU();
                objektit[*TOI[1]]->load_from_buf1_to_GPU();
                objektit[*TOI[2]]->load_from_buf2_to_GPU();
                objektit[*TOI[2]]->load_from_buf1_to_GPU();

                //next insta matriisit samoille TOI paikoille
                //cubet[*TOI[0]].bind_instance_model_matrixes(objektit[*TOI[0]]->laatikko_insta_models_next);

                int temp1, temp2, temp3;
                temp1 = *TOI[6];
                temp2 = *TOI[7];
                temp3 = *TOI[8];
                *TOI[6] = *TOI[0];
                *TOI[7] = *TOI[1];
                *TOI[8] = *TOI[2];
                *TOI[0] = *TOI[3];
                *TOI[1] = *TOI[4];
                *TOI[2] = *TOI[5];
                *TOI[3] = temp1;
                *TOI[4] = temp2;
                *TOI[5] = temp3;

                objektit[*TOI[0]]->neednewmaps = true;
                objektit[*TOI[1]]->neednewmaps = true;
                objektit[*TOI[2]]->neednewmaps = true;
                //objektit[*TOI[6]]->neednewmaps = true;
                //objektit[*TOI[7]]->neednewmaps = true;
                //objektit[*TOI[8]]->neednewmaps = true;
            }
        }
        else if (nig == glm::vec2(0, 1) && kamera.cameraTILE[1] > 0)
        {
            if (abs((int)kamera.cameraTILE[1] % 2) == 1)
            {
                //std::cout << "GENUP" << std::endl;
                objektit[*TOI[6]]->position[2] += 12000;
                objektit[*TOI[6]]->trans_obj(objektit[*TOI[6]]->position);
                objektit[*TOI[7]]->position[2] += 12000;
                objektit[*TOI[7]]->trans_obj(objektit[*TOI[7]]->position);
                objektit[*TOI[8]]->position[2] += 12000;
                objektit[*TOI[8]]->trans_obj(objektit[*TOI[8]]->position);
                objektit[*TOI[6]]->load_from_buf2_to_GPU();
                objektit[*TOI[6]]->load_from_buf1_to_GPU();

                objektit[*TOI[7]]->load_from_buf2_to_GPU();
                objektit[*TOI[7]]->load_from_buf1_to_GPU();

                objektit[*TOI[8]]->load_from_buf2_to_GPU();
                objektit[*TOI[8]]->load_from_buf1_to_GPU();
                int temp1, temp2, temp3;

                temp1 = *TOI[0];
                temp2 = *TOI[1];
                temp3 = *TOI[2];

                *TOI[0] = *TOI[6];
                *TOI[1] = *TOI[7];
                *TOI[2] = *TOI[8];

                *TOI[6] = *TOI[3];
                *TOI[7] = *TOI[4];
                *TOI[8] = *TOI[5];

                *TOI[3] = temp1;
                *TOI[4] = temp2;
                *TOI[5] = temp3;

                objektit[*TOI[0]]->neednewmaps = true;
                objektit[*TOI[1]]->neednewmaps = true;
                objektit[*TOI[2]]->neednewmaps = true;

                
                //objektit[*TOI[6]]->neednewmaps = true;
                //objektit[*TOI[7]]->neednewmaps = true;
                //objektit[*TOI[8]]->neednewmaps = true;
                
            }
        }
        else if (nig == glm::vec2(0, 1) && kamera.cameraTILE[1] <= 0)
        {
            if (abs((int)kamera.camera_LAST_TILE[1] % 2) == 1)
            {
                //std::cout << "GENUP" << std::endl;
                objektit[*TOI[6]]->position[2] += 12000;
                objektit[*TOI[6]]->trans_obj(objektit[*TOI[6]]->position);
                objektit[*TOI[7]]->position[2] += 12000;
                objektit[*TOI[7]]->trans_obj(objektit[*TOI[7]]->position);
                objektit[*TOI[8]]->position[2] += 12000;
                objektit[*TOI[8]]->trans_obj(objektit[*TOI[8]]->position);
                objektit[*TOI[6]]->load_from_buf2_to_GPU();
                objektit[*TOI[6]]->load_from_buf1_to_GPU();

                objektit[*TOI[7]]->load_from_buf2_to_GPU();
                objektit[*TOI[7]]->load_from_buf1_to_GPU();

                objektit[*TOI[8]]->load_from_buf2_to_GPU();
                objektit[*TOI[8]]->load_from_buf1_to_GPU();
                int temp1, temp2, temp3;

                temp1 = *TOI[0];
                temp2 = *TOI[1];
                temp3 = *TOI[2];

                *TOI[0] = *TOI[6];
                *TOI[1] = *TOI[7];
                *TOI[2] = *TOI[8];

                *TOI[6] = *TOI[3];
                *TOI[7] = *TOI[4];
                *TOI[8] = *TOI[5];

                *TOI[3] = temp1;
                *TOI[4] = temp2;
                *TOI[5] = temp3;

                objektit[*TOI[0]]->neednewmaps = true;
                objektit[*TOI[1]]->neednewmaps = true;
                objektit[*TOI[2]]->neednewmaps = true;

                //objektit[*TOI[6]]->neednewmaps = true;
                //objektit[*TOI[7]]->neednewmaps = true;
                //objektit[*TOI[8]]->neednewmaps = true;
            }
        }
        else if (nig == glm::vec2(-1, 0) && kamera.cameraTILE[0] >= 0)
        {
            if (abs((int)kamera.camera_LAST_TILE[0] % 2) == 1)
            {
                //std::cout << "GENLEFT" << std::endl;
                objektit[*TOI[2]]->position[0] -= 12000;
                objektit[*TOI[2]]->trans_obj(objektit[*TOI[2]]->position);
                objektit[*TOI[5]]->position[0] -= 12000;
                objektit[*TOI[5]]->trans_obj(objektit[*TOI[5]]->position);
                objektit[*TOI[8]]->position[0] -= 12000;
                objektit[*TOI[8]]->trans_obj(objektit[*TOI[8]]->position);

                objektit[*TOI[2]]->load_from_buf2_to_GPU();
                objektit[*TOI[2]]->load_from_buf1_to_GPU();

                objektit[*TOI[5]]->load_from_buf2_to_GPU();
                objektit[*TOI[5]]->load_from_buf1_to_GPU();

                objektit[*TOI[8]]->load_from_buf2_to_GPU();
                objektit[*TOI[8]]->load_from_buf1_to_GPU();

                int temp1, temp2, temp3;
                temp1 = *TOI[0];
                temp2 = *TOI[3];
                temp3 = *TOI[6];

                *TOI[0] = *TOI[2];
                *TOI[3] = *TOI[5];
                *TOI[6] = *TOI[8];

                *TOI[2] = *TOI[1];
                *TOI[5] = *TOI[4];
                *TOI[8] = *TOI[7];

                *TOI[1] = temp1;
                *TOI[4] = temp2;
                *TOI[7] = temp3;

                objektit[*TOI[0]]->neednewmaps = true;
                objektit[*TOI[3]]->neednewmaps = true;
                objektit[*TOI[6]]->neednewmaps = true;

                //objektit[*TOI[2]]->neednewmaps = true;
                //objektit[*TOI[5]]->neednewmaps = true;
                //objektit[*TOI[8]]->neednewmaps = true;

            }
        }
        else if (nig == glm::vec2(-1, 0) && kamera.cameraTILE[0] < 0)
        {
            if (abs((int)kamera.cameraTILE[0] % 2) == 1)
            {
                //std::cout << "GENLEFT" << std::endl;
                objektit[*TOI[2]]->position[0] -= 12000;
                objektit[*TOI[2]]->trans_obj(objektit[*TOI[2]]->position);
                objektit[*TOI[5]]->position[0] -= 12000;
                objektit[*TOI[5]]->trans_obj(objektit[*TOI[5]]->position);
                objektit[*TOI[8]]->position[0] -= 12000;
                objektit[*TOI[8]]->trans_obj(objektit[*TOI[8]]->position);

                objektit[*TOI[2]]->load_from_buf2_to_GPU();
                objektit[*TOI[2]]->load_from_buf1_to_GPU();

                objektit[*TOI[5]]->load_from_buf2_to_GPU();
                objektit[*TOI[5]]->load_from_buf1_to_GPU();

                objektit[*TOI[8]]->load_from_buf2_to_GPU();
                objektit[*TOI[8]]->load_from_buf1_to_GPU();
                int temp1, temp2, temp3;
                temp1 = *TOI[0];
                temp2 = *TOI[3];
                temp3 = *TOI[6];

                *TOI[0] = *TOI[2];
                *TOI[3] = *TOI[5];
                *TOI[6] = *TOI[8];

                *TOI[2] = *TOI[1];
                *TOI[5] = *TOI[4];
                *TOI[8] = *TOI[7];

                *TOI[1] = temp1;
                *TOI[4] = temp2;
                *TOI[7] = temp3;
                objektit[*TOI[0]]->neednewmaps = true;
                objektit[*TOI[3]]->neednewmaps = true;
                objektit[*TOI[6]]->neednewmaps = true;

                //objektit[*TOI[2]]->neednewmaps = true;
                //objektit[*TOI[5]]->neednewmaps = true;
               // objektit[*TOI[8]]->neednewmaps = true;
            }
        }
        else if (nig == glm::vec2(1, 0) && kamera.cameraTILE[0] > 0)
        {
            if (abs((int)kamera.cameraTILE[0] % 2) == 1)
            {
                //std::cout << "GENRIGHT" << std::endl;
                objektit[*TOI[0]]->position[0] += 12000;
                objektit[*TOI[0]]->trans_obj(objektit[*TOI[0]]->position);
                objektit[*TOI[3]]->position[0] += 12000;
                objektit[*TOI[3]]->trans_obj(objektit[*TOI[3]]->position);
                objektit[*TOI[6]]->position[0] += 12000;
                objektit[*TOI[6]]->trans_obj(objektit[*TOI[6]]->position);

                objektit[*TOI[0]]->load_from_buf2_to_GPU();
                objektit[*TOI[0]]->load_from_buf1_to_GPU();

                objektit[*TOI[3]]->load_from_buf2_to_GPU();
                objektit[*TOI[3]]->load_from_buf1_to_GPU();

                objektit[*TOI[6]]->load_from_buf2_to_GPU();
                objektit[*TOI[6]]->load_from_buf1_to_GPU();
                int temp1, temp2, temp3;
                temp1 = *TOI[2];
                temp2 = *TOI[5];
                temp3 = *TOI[8];

                *TOI[2] = *TOI[0];
                *TOI[5] = *TOI[3];
                *TOI[8] = *TOI[6];

                *TOI[0] = *TOI[1];
                *TOI[3] = *TOI[4];
                *TOI[6] = *TOI[7];

                *TOI[1] = temp1;
                *TOI[4] = temp2;
                *TOI[7] = temp3;
                objektit[*TOI[0]]->neednewmaps = true;
                objektit[*TOI[3]]->neednewmaps = true;
                objektit[*TOI[6]]->neednewmaps = true;

                //objektit[*TOI[2]]->neednewmaps = true;
                //objektit[*TOI[5]]->neednewmaps = true;
                //objektit[*TOI[8]]->neednewmaps = true;
            }
        }
        else if (nig == glm::vec2(1, 0) && kamera.cameraTILE[0] <= 0)
        {
            if (abs((int)kamera.camera_LAST_TILE[0] % 2) == 1)
            {
                //std::cout << "GENRIGHT" << std::endl;
                objektit[*TOI[0]]->position[0] += 12000;
                objektit[*TOI[0]]->trans_obj(objektit[*TOI[0]]->position);
                objektit[*TOI[3]]->position[0] += 12000;
                objektit[*TOI[3]]->trans_obj(objektit[*TOI[3]]->position);
                objektit[*TOI[6]]->position[0] += 12000;
                objektit[*TOI[6]]->trans_obj(objektit[*TOI[6]]->position);

                objektit[*TOI[0]]->load_from_buf2_to_GPU();
                objektit[*TOI[0]]->load_from_buf1_to_GPU();

                objektit[*TOI[3]]->load_from_buf2_to_GPU();
                objektit[*TOI[3]]->load_from_buf1_to_GPU();

                objektit[*TOI[6]]->load_from_buf2_to_GPU();
                objektit[*TOI[6]]->load_from_buf1_to_GPU();
                int temp1, temp2, temp3;
                temp1 = *TOI[2];
                temp2 = *TOI[5];
                temp3 = *TOI[8];

                *TOI[2] = *TOI[0];
                *TOI[5] = *TOI[3];
                *TOI[8] = *TOI[6];

                *TOI[0] = *TOI[1];
                *TOI[3] = *TOI[4];
                *TOI[6] = *TOI[7];

                *TOI[1] = temp1;
                *TOI[4] = temp2;
                *TOI[7] = temp3;
                objektit[*TOI[0]]->neednewmaps = true;
                objektit[*TOI[3]]->neednewmaps = true;
                objektit[*TOI[6]]->neednewmaps = true;

                //objektit[*TOI[2]]->neednewmaps = true;
                //objektit[*TOI[5]]->neednewmaps = true;
                //objektit[*TOI[8]]->neednewmaps = true;
            }
        }
    }
    kamera.camera_LAST_TILE = kamera.cameraTILE;

    //initial tileorder
    //0,1,2
    //3,4,5
    //6,7,8

    //render
    for (short i = 0; i < objektit.size(); i++)
    {
        objektit[i]->send_model_uniforms(shader);
        //objektit[i]->send_TI_model_uniforms(shader);
        objektit[i]->draw_tesselated_obj(shader);
    }
    glUseProgram(0);
}

void render_terrain_shadow(unsigned int shader, std::vector<TERRAIN_OBJEKTI*> objektit, glm::mat4 lightSpaceMatrix)
{
    glUseProgram(shader);

    SetUniVec3(shader, kamera.cameraPos, "gEyeWorldPos");
    SetUniMat4(shader, lightSpaceMatrix, "PV");
    for (short i = 0; i < objektit.size(); i++)
    {
        objektit[i]->send_model_uniforms(shader);
        //objektit[i]->send_TI_model_uniforms(shader);
        objektit[i]->draw_tesselated_obj(shader);
    }
    glUseProgram(0);
}

void render_terrain_instanced_objs(unsigned int shader, std::vector<OBJEKTI*>objektit,glm::mat4 projection)
{
    glUseProgram(shader);
    glDisable(GL_CULL_FACE);
    SetUniVec3(shader, kamera.cameraPos, "gEyeWorldPos");
    SetUniMat4(shader, (projection * kamera.getview()), "PV");
    for (auto x : objektit)
    {
        x->draw_obj_instanced(shader);
    }
    glEnable(GL_CULL_FACE);
    glUseProgram(0);
}

void render_terrain_tesselated_instanced_objs(unsigned int shader, std::vector<OBJEKTI*>puskat, glm::mat4 projection)
{
    glUseProgram(shader);
    glDisable(GL_CULL_FACE);
    SetUniVec3(shader, kamera.cameraPos, "gEyeWorldPos");
    SetUniMat4(shader, (projection * kamera.getview()), "PV");
    for (auto x : puskat)
    {
        x->draw_tesselated_instanced_obj(shader);
    }
    glEnable(GL_CULL_FACE);
    glUseProgram(0);
}

void render_screen(unsigned int shader, OBJEKTI screen, FBO FBO)
{
    glUseProgram(shader);
    screen.send_model_uniforms(shader);
    screen.draw_obj(shader, FBO.text_id);
    glUseProgram(0);
}

void render_screen_GBUF(unsigned int shader, OBJEKTI screen, unsigned int buftext)
{
    glUseProgram(shader);
    screen.send_model_uniforms(shader);
    screen.draw_obj(shader, buftext);
    glUseProgram(0);
}

std::vector<TERRAIN_OBJEKTI> TERO()
{
    TERRAIN_OBJEKTI floor1;
    floor1.load_model("res/objects/lvl6.obj");
    floor1.load_textures("res/textures/grass_texture.png");
    floor1.load_normals("res/textures/h222norm.png");
    floor1.load_normals2("res/textures/grass_normal.png");
    floor1.load_displacement("res/temp/temp0.png");
    floor1.load_terrain_rock_text("res/textures/rocktex.png");
    floor1.load_ter_rock_normal("res/textures/laava_normal.png");
    floor1.position = glm::vec3(-4000.0f, 0.0f, 4000.0f);
    floor1.trans_obj(glm::vec3(-4000.0f, 0.0f, 4000.0f));
    floor1.load_displacement_to_buf1("res/temp/temp1.png");
    //floor1.printbuf();
    

    TERRAIN_OBJEKTI floor2;
    floor2.load_model("res/objects/lvl6.obj");
    floor2.load_textures("res/textures/grass_texture.png");
    floor2.load_normals("res/textures/h222norm.png");
    floor2.load_normals2("res/textures/grass_normal.png");
    floor2.load_displacement("res/temp/temp1.png");
    floor2.load_terrain_rock_text("res/textures/rocktex.png");
    floor2.load_ter_rock_normal("res/textures/laava_normal.png");
    floor2.position = glm::vec3(0.0f, 0.0f, 4000.0f);
    floor2.trans_obj(glm::vec3(0.0f, 0.0f, 4000.0f));
    floor2.load_displacement_to_buf1("res/temp/temp1.png");

    TERRAIN_OBJEKTI floor3;
    floor3.load_model("res/objects/lvl6.obj");
    floor3.load_textures("res/textures/grass_texture.png");
    floor3.load_normals("res/textures/h222norm.png");
    floor3.load_normals2("res/textures/grass_normal.png");
    floor3.load_displacement("res/temp/temp2.png");
    floor3.load_terrain_rock_text("res/textures/rocktex.png");
    floor3.load_ter_rock_normal("res/textures/laava_normal.png");
    floor3.position = glm::vec3(4000.0f, 0.0f, 4000.0f);
    floor3.trans_obj(glm::vec3(4000.0f, 0.0f, 4000.0f));
    floor3.load_displacement_to_buf1("res/temp/temp2.png");

    TERRAIN_OBJEKTI floor4;
    floor4.load_model("res/objects/lvl6.obj");
    floor4.load_textures("res/textures/grass_texture.png");
    floor4.load_normals("res/textures/h222norm.png");
    floor4.load_normals2("res/textures/grass_normal.png");
    floor4.load_displacement("res/temp/temp3.png");
    floor4.load_terrain_rock_text("res/textures/rocktex.png");
    floor4.load_ter_rock_normal("res/textures/laava_normal.png");
    floor4.position = glm::vec3(-4000.0f, 0.0f, 0.0f);
    floor4.trans_obj(glm::vec3(-4000.0f, 0.0f, 0.0f));
    floor4.load_displacement_to_buf1("res/temp/temp3.png");

    TERRAIN_OBJEKTI floor5;
    floor5.load_model("res/objects/lvl6.obj");
    floor5.load_textures("res/textures/grass_texture.png");
    floor5.load_normals("res/textures/h222norm.png");
    floor5.load_normals2("res/textures/grass_normal.png");
    floor5.load_displacement("res/temp/temp4.png");
    floor5.load_terrain_rock_text("res/textures/rocktex.png");
    floor5.load_ter_rock_normal("res/textures/laava_normal.png");
    floor5.position = glm::vec3(0.0f, 0.0f, 0.0f);
    floor5.trans_obj(glm::vec3(0.0f, 0.0f, 0.0f));
    floor5.load_displacement_to_buf1("res/temp/temp4.png");

    TERRAIN_OBJEKTI floor6;
    floor6.load_model("res/objects/lvl6.obj");
    floor6.load_textures("res/textures/grass_texture.png");
    floor6.load_normals("res/textures/h222norm.png");
    floor6.load_normals2("res/textures/grass_normal.png");
    floor6.load_displacement("res/temp/temp5.png");
    floor6.load_terrain_rock_text("res/textures/rocktex.png");
    floor6.load_ter_rock_normal("res/textures/laava_normal.png");
    floor6.position = glm::vec3(4000.0f, 0.0f, 0.0f);
    floor6.trans_obj(glm::vec3(4000.0f, 0.0f, 0.0f));
    floor6.load_displacement_to_buf1("res/temp/temp5.png");

    TERRAIN_OBJEKTI floor7;
    floor7.load_model("res/objects/lvl6.obj");
    floor7.load_textures("res/textures/grass_texture.png");
    floor7.load_normals("res/textures/h222norm.png");
    floor7.load_normals2("res/textures/grass_normal.png");
    floor7.load_displacement("res/temp/temp6.png");
    floor7.load_terrain_rock_text("res/textures/rocktex.png");
    floor7.load_ter_rock_normal("res/textures/laava_normal.png");
    floor7.position = glm::vec3(-4000.0f, 0.0f, -4000.0f);
    floor7.trans_obj(glm::vec3(-4000.0f, 0.0f, -4000.0f));
    floor7.load_displacement_to_buf1("res/temp/temp6.png");

    TERRAIN_OBJEKTI floor8;
    floor8.load_model("res/objects/lvl6.obj");
    floor8.load_textures("res/textures/grass_texture.png");
    floor8.load_normals("res/textures/h222norm.png");
    floor8.load_normals2("res/textures/grass_normal.png");
    floor8.load_displacement("res/temp/temp7.png");
    floor8.load_terrain_rock_text("res/textures/rocktex.png");
    floor8.load_ter_rock_normal("res/textures/laava_normal.png");
    floor8.position = glm::vec3(0.0f, 0.0f, -4000.0f);
    floor8.trans_obj(glm::vec3(0.0f, 0.0f, -4000.0f));
    floor8.load_displacement_to_buf1("res/temp/temp7.png");

    TERRAIN_OBJEKTI floor9;
    floor9.load_model("res/objects/lvl6.obj");
    floor9.load_textures("res/textures/grass_texture.png");
    floor9.load_normals("res/textures/h222norm.png");
    floor9.load_normals2("res/textures/grass_normal.png");
    floor9.load_displacement("res/temp/temp8.png");
    floor9.load_terrain_rock_text("res/textures/rocktex.png");
    floor9.load_ter_rock_normal("res/textures/laava_normal.png");
    floor9.position = glm::vec3(4000.0f, 0.0f, -4000.0f);
    floor9.trans_obj(glm::vec3(4000.0f, 0.0f, -4000.0f));
    floor9.load_displacement_to_buf1("res/temp/temp8.png");

    return {floor1,floor2,floor3,floor4,floor5,floor6,floor7,floor8,floor9};
}

void terraincreator(std::vector<int*> TOI, std::vector<TERRAIN_OBJEKTI*> terrainobj, bool* running,int threadindex)
{
    //TOI
    //0,1,2
    //3,4,5
    //6,7,8
    srand(threadindex);

    std::vector<int> TOI_lista(9);
    hMapGen hmgen;
    
    //ONGELMAHAN ON KUN MENEE NOPEESTI 2x rajan yli nii bufferit ei ehi kirjottuun
    //lisataanko 1 bufferi etta rajan yli voi menna 2 kertaa vai enemman buffereja?
    //joku parempi? 

    while (*running)
    {
        if (terrainobj[threadindex]->neednewmaps)
        {
            hmgen.createheightMap8bit(1024,1024,1);
            hmgen.writePNG(("res/temp/temp"+std::to_string(threadindex)+".png").c_str());

            terrainobj[threadindex]->load_displacement_to_buf1(("res/temp/temp" + std::to_string(threadindex) + ".png").c_str());
            //gen next possible instanssi matriisit seuraavaks

            //nopeesti otetaan mutexilla TOI lista itelle talteen

            TOI_mutex.lock();
            terrainobj[threadindex]->neednewmaps = false;
            for (int i = 0; i < TOI.size(); i++)
            {
                TOI_lista[i] = *TOI[i];
            }
            TOI_mutex.unlock();

            //generate heightmap
            //generate blendmap
            //generate instanssi matriisit

            //load hm to buffer
            //load blend to buffers
        }
        //laitetaan threadi nukkuun jos ei tarvi uutta mappia
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

}

int main(void)
{
    //konsolipiiloon();
    GLFWwindow* window = init_windowJaOpenGL();
    ImGuiIO& io = ImGui::GetIO(); (void)io;


    GLint MaxPatchVertices = 0;
    glGetIntegerv(GL_MAX_PATCH_VERTICES, &MaxPatchVertices);
    //printf("Max supported patch vertices %d\n", MaxPatchVertices);
    glPatchParameteri(GL_PATCH_VERTICES, 3);

    unsigned int simpshader = make_shader("res/shaders/simpshader.shader",false);
    unsigned int screenshader = make_shader("res/shaders/screenshader.shader", false);
    unsigned int pelkkatex = make_shader("res/shaders/pelkkatex.shader", false);
    unsigned int terrainshader = make_shader("res/shaders/terrain.shader", false);
    unsigned int tessShader = make_shader("res/shaders/simplestess.shader", true);
    unsigned int tessWireShader = make_shader("res/shaders/tesswire.shader", true);
    unsigned int simpleinstanced = make_shader("res/shaders/simpleinstanced.shader", false);
    unsigned int tesselatedinstanced = make_shader("res/shaders/tesselatedinstanced.shader", true);
    unsigned int ITW = make_shader("res/shaders/instanssitesswire.shader", true);
    unsigned int GBTTS = make_shader("res/shaders/GBufferTesselatedTerrainShader.shader", true);
    unsigned int GBSI = make_shader("res/shaders/GbufferSimpleInstanced.shader", false);
    unsigned int Glightpass = make_shader("res/shaders/Glightpass.shader", false);
    unsigned int terrainshadow = make_shader("res/shaders/terrainshadow.shader", true);

    glUseProgram(0);



    OBJEKTI default_text;
    default_text.load_textures("res/textures/default.png");

    std::vector<OBJEKTI> cubet;
    for (int i = 0; i < 9; i++)
    {
        OBJEKTI puska;
        puska.load_model("res/objects/nurmikko.obj");
        puska.load_textures("res/textures/brantsi_lehti.png");
        puska.load_normals("res/textures/brantsi_lehti_normal.png");
        puska.trans_obj(glm::vec3(0, 0, 0));
        cubet.emplace_back(puska);
    }
    std::vector<OBJEKTI> tessinstabush;
    for (int i = 0; i < 9; i++)
    {
        OBJEKTI puska2;
        puska2.load_model("res/objects/nurmikko.obj");
        puska2.load_displacement("res/textures/disptest.png");
        puska2.load_textures("res/textures/brantsi_lehti.png");
        puska2.load_normals("res/textures/brantsi_lehti_normal.png");
        puska2.trans_obj(glm::vec3(0, 0, 0));
        tessinstabush.emplace_back(puska2);
    }


    OBJEKTI cube;
    cube.load_model("res/objects/kuutio.obj");
    cube.load_textures("res/textures/carbon.png");
    cube.trans_obj(glm::vec3(0, 100, 0));

    OBJEKTI valo;
    valo.load_model("res/objects/kuutio.obj");
    valo.trans_obj(glm::vec3(0, 1000.0, 0));
    valo.scale_obj(0.2f);

    OBJEKTI skybox;
    skybox.load_model("res/objects/skybox2.obj");
    skybox.load_textures("res/textures/clouds3.jpg");
    skybox.scale_obj(7000);

    OBJEKTI main_screen;
    main_screen.load_model("res/objects/Mscr.obj");
    main_screen.trans_obj(glm::vec3(-1.0, 0.0, 0.0));

    OBJEKTI tiny_screen1;
    tiny_screen1.load_model("res/objects/Mscr.obj");
    tiny_screen1.scale_obj(0.5f);
    tiny_screen1.trans_obj(glm::vec3(0.0, 0.5, 0.0));

    OBJEKTI tiny_screen2;
    tiny_screen2.load_model("res/objects/Mscr.obj");
    tiny_screen2.scale_obj(0.5f);
    tiny_screen2.trans_obj(glm::vec3(0.5, 0.5, 0.0));

    OBJEKTI tiny_screen3;
    tiny_screen3.load_model("res/objects/Mscr.obj");
    tiny_screen3.scale_obj(0.5f);
    tiny_screen3.trans_obj(glm::vec3(0.0, -0.5, 0.0));

    OBJEKTI tiny_screen4;
    tiny_screen4.load_model("res/objects/Mscr.obj");
    tiny_screen4.scale_obj(0.5f);
    tiny_screen4.trans_obj(glm::vec3(0.5, -0.5, 0.0));

    OBJEKTI lightpass_screen;
    lightpass_screen.load_model("res/objects/bufferscreen.obj");


    FBO mainscreen = create_FBO(800,600,"COLOR");
    FBO nolighting = create_FBO(800, 600, "COLOR");
    FBO shadows = create_FBO(1024, 1024, "DEPTH");
    GBUFFER Gbuffer = create_Gbuffer(800, 600);

    int ww, wh;
    glfwGetWindowSize(window, &ww, &wh);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f/600.0f, 0.15f, 200.f);


    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    const char* glsl_version = "#version 410";
    ImGui_ImplOpenGL3_Init(glsl_version);
    io.Fonts->AddFontDefault();

    ImGuiData floats;
    floats.xrot = 0.0f;
    floats.yrot = 0.0f;
    floats.zrot = 0.0f;
    
    glEnable(GL_DEPTH_TEST);
    //glDepthFunc(GL_LEQUAL);
    //glEnable(GL_MULTISAMPLE);


    glm::vec3 Lpos = glm::vec3(0, 1000.0, 0);

    glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 10.0f, 200.0f);
    glm::vec3 SUN_POS = kamera.cameraPos + glm::vec3(0, 1000.0, 1000.0);
    glm::mat4 lightView = glm::lookAt(SUN_POS, kamera.cameraPos, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    glUseProgram(simpshader);
    SetUniVec3(simpshader, Lpos, "lightpos");
    glUseProgram(0);

    glUseProgram(pelkkatex);
    SetUniVec3(pelkkatex, Lpos, "lightpos");
    glUseProgram(0);

    glUseProgram(terrainshader);
    SetUniVec3(terrainshader, Lpos, "lightpos");
    glUseProgram(0);

    glUseProgram(tessShader);
    SetUniVec3(tessShader, Lpos, "lightpos");
    glUseProgram(0);

    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    std::vector<OBJEKTI*> objektit = { &cube,&skybox,&valo };

    hMapGen GENE;

    std::vector<TERRAIN_OBJEKTI*> terrainobj;
    std::vector<TERRAIN_OBJEKTI> terrainobj_real = TERO();
    for (int i = 0;i < terrainobj_real.size(); i++)
    {
        terrainobj_real[i].load_blendmap_to_buf2("res/textures/grass_texture.png"); // joo tatakaan ei sit oikeesti tassa loadata lol
        terrainobj.emplace_back(&terrainobj_real[i]);
    }

    std::vector<int*> TOI;
    std::vector<int> terrain_indexes = {0,1,2,3,4,5,6,7,8};
    for (int i = 0; i < terrain_indexes.size(); i++)
    {
        TOI.emplace_back(&terrain_indexes[i]);
    }

    std::vector<OBJEKTI*> cubeinstat;
    for (int i = 0; i < cubet.size(); i++)
    {
        cubet[i].bind_instance_model_matrixes(terrainobj[i]->generate_laatikko_insta_models());
        cubeinstat.emplace_back(&cubet[i]);
    }
    std::vector<OBJEKTI*> tessinstapuska;
    for (int i = 0; i < cubet.size(); i++)
    {
        tessinstabush[i].bind_instance_model_matrixes(terrainobj[i]->generate_laatikko_insta_models());
        tessinstapuska.emplace_back(&tessinstabush[i]);
    }

    //launchataan threadit jotka muokkaa terraineja
    std::thread terrainedithreads[9];
    bool running = true;
    for (int i = 0; i < 9; i++)
    {
        terrainedithreads[i] = std::thread(terraincreator, TOI, terrainobj, &running,i);
    }

    while (!glfwWindowShouldClose(window))
    {
        projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.15f, 50000.f);

        
        /*
        glBindFramebuffer(GL_FRAMEBUFFER,mainscreen.id);
        glViewport(0, 0, 800.0f, 600.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        render_scene_simpshader(simpshader, objektit, floats, projection);
        render_terrain(tessShader, terrainobj, projection, mainscreen, TOI);
        render_terrain_instanced_objs(simpleinstanced, cubeinstat,projection);
        //render_terrain_tesselated_instanced_objs(tesselatedinstanced, tessinstapuska, projection);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        */

        glViewport(0, 0, 800.0f, 600.0f);
        glBindFramebuffer(GL_FRAMEBUFFER, Gbuffer.gBuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        render_terrain(GBTTS, terrainobj, projection, mainscreen, TOI);
        render_terrain_instanced_objs(GBSI, cubeinstat, projection);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        // VITUNVITUNVITUN VITTUPÄIVÄT.. ELIKKAS SITTENKIN KANNATTAA TEHA GBUFFERISSA TAA HASSAKKA KOSKA FRAGPOSLIGHTSPACE
        SUN_POS = kamera.cameraPos + glm::vec3(50.0, 50.0, 50.0);
        lightView = glm::lookAt(SUN_POS, kamera.cameraPos, glm::vec3(0.0f, 1.0f, 0.0f));
        lightSpaceMatrix = lightProjection * lightView;
        glViewport(0, 0, 1024.0f, 1024.0f);
        glBindFramebuffer(GL_FRAMEBUFFER, shadows.id);
        glClear(GL_DEPTH_BUFFER_BIT);
        render_terrain_shadow(terrainshadow, terrainobj, lightSpaceMatrix);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        
        glViewport(0, 0, 800.0f, 600.0f);
        glBindFramebuffer(GL_FRAMEBUFFER, mainscreen.id);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        lightpass_screen.draw_lightpass(Glightpass, Gbuffer.gPosition, Gbuffer.gNormal, Gbuffer.gColor, kamera.cameraPos,projection, lightProjection ,lightView,shadows);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        
        /*
        glBindFramebuffer(GL_FRAMEBUFFER, nolighting.id);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        render_scene_nolighting(pelkkatex, objektit, floats, projection);
        glDisable(GL_CULL_FACE);
        render_terrain(tessWireShader, terrainobj, projection, mainscreen, TOI);
        //render_terrain_tesselated_instanced_objs(ITW, tessinstapuska, projection);
        glEnable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        */


        glViewport(0, 0, 1600.0f, 600.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        render_screen(screenshader, main_screen, mainscreen);
        //render_screen(screenshader, tiny_screen1, nolighting);
        render_screen_GBUF(screenshader, tiny_screen1, shadows.depth);
        render_screen_GBUF(screenshader, tiny_screen2, Gbuffer.gNormal);
        render_screen_GBUF(screenshader, tiny_screen3, Gbuffer.gColor);
        render_screen_GBUF(screenshader, tiny_screen4, Gbuffer.gPosition);
        //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        

        floats = imgui_ruutu(window, floats);
        processInput(window);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glUseProgram(0);
    glDeleteProgram(simpshader);
    glfwTerminate();
    TOI_mutex.lock();
    running = false;
    TOI_mutex.unlock();

    for (int i = 0; i < 9; i++)
    {
        terrainedithreads[i].join();
    }
    return 0;
}