#include "dream.h"
#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <map>
#include <vector>
#include <fstream>
#include "stb_image.h"
GLFWwindow* window;
unsigned short screenWidth =800;
unsigned short screenHeight =600;
unsigned int vao;
unsigned int quadVbo;

float delta = 0.0f;

std::map<std::string, Shader*> shaderCache;
std::map<std::string, Texture*> textureCache;

float quadVerts[] = {
    -0.5f,-0.5f,0.25f,        0.0f,0.0f,
    0.5f,-0.5f,0.25f,         1.0f,0.0f,
    0.5f,0.5f,0.25f,          1.0f,1.0f,

    -0.5f,0.5f,0.25f,         0.0f,1.0f,
    -0.5f,-0.5f,0.25f,        0.0f,0.0f,
    0.5f,0.5f,0.25f,          1.0f,1.0f
};

std::vector<DrawCall*> renderQueue;

GameObject* sceneRoot = nullptr;

float start = 0.0f;

void (*curInitFunc)() = nullptr;
void (*curLoopFunc)(float) = nullptr;

Texture::Texture(std::string path)
{
    int width,height,nChannels;
    unsigned char* data = stbi_load(path.c_str(),&width,&height,&nChannels,0);
    if(data)
    {
        unsigned int texture;
        glGenTextures(1,&texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE,data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
    }
    else
    {
        std::cout << "Failed to load texture: " << path << "\n";
    }
}

void DreamAddGameObject(GameObject* object)
{
    sceneRoot->AddChild(object);
}

void GameObject::AddChild(GameObject* object)
{
    children.push_back(object);
}


Texture* GetTexture(std::string path)
{
    if(textureCache.find(path) != textureCache.end())
    {
        return textureCache.at(path);
    }
    else
    {
        Texture* texture = new Texture(path);
        textureCache.emplace(path,texture);
        return texture;
    }
}

Shader* GetShader(std::string path)
{
    if(shaderCache.find(path) != shaderCache.end())
    {
        return shaderCache.at(path);
    }
    else
    {
        Shader* shader = new Shader(path);
        shaderCache.emplace(path, shader);
        return shader;
    }
}


void Shader::bind()
{
    glUseProgram(this->shaderHandle);
}


Shader::Shader(std::string path)
{
    std::string vertexSource;
    std::string fragmentSource;

    std::ifstream vertFile("shaders/" + path + ".vsl");
    if(!vertFile.is_open())
    {
        std::cout << "Failed to find file: " + path + ".vsl\n";
    }

    std::string line;

    while(std::getline(vertFile,line))
    {
        vertexSource += line;
        vertexSource += "\n";
    }

    std::ifstream fragFile("shaders/" + path + ".fsl");

    if(!fragFile.is_open())
    {
        std::cout << "Failed to find file: " + path + ".fsl\n";
    }

    while(std::getline(fragFile,line))
    {
        fragmentSource += line;
        fragmentSource += "\n";
    }

    const char* vertSrc = vertexSource.c_str();
    const char* fragSrc = fragmentSource.c_str();


    int success;

    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader,1,&vertSrc,NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader,512,NULL,infoLog);
        std::cout << "ERROR: Vertex Shader Compilation Failed\n\tShader:\t" << path << "\n\t"<<infoLog<<"\n";
    }


    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader,1,&fragSrc,NULL);
    glCompileShader(fragmentShader);
    if(!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader,512,NULL,infoLog);
        std::cout << "ERROR: Fragment Shader Compilation Failed\n\tShader:\t" << path << "\n\t"<<infoLog<<"\n";
    }


    shaderHandle = glCreateProgram();

    glAttachShader(shaderHandle,vertexShader);
    glAttachShader(shaderHandle, fragmentShader);

    glLinkProgram(shaderHandle);
}



DrawCall::~DrawCall()
{

}



void DreamInitFunc(void (*initFunc)())
{
    curInitFunc = initFunc;
}

void DreamMainLoopFunc(void (*loopFunc)(float))
{
    curLoopFunc = loopFunc;
}

void DreamProcessRenderQueue()
{
    for(DrawCall* d : renderQueue)
    {
        d->draw();
        delete d;
    }
    renderQueue.clear();
}


void DreamStart()
{
    DreamInit();
    curInitFunc();
    while(!glfwWindowShouldClose((window)))
    {
        start = glfwGetTime();
        glfwPollEvents();

        curLoopFunc(delta);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if(sceneRoot != nullptr)
        {
            sceneRoot->processTick(delta);
            sceneRoot->processRender();
        }
        DreamProcessRenderQueue();

        glfwSwapBuffers(window);
        delta = glfwGetTime() - start;
    }
    DreamTerminate();
}


void DreamInit()
{
    if(!glfwInit())
    {
        std::cout << "glfw initialization failed. try again jackass.\n";
    }
    else
    {
        window = glfwCreateWindow(screenWidth,screenHeight,"Dream - v0.0.2",NULL,NULL);
        glfwMakeContextCurrent(window);
        glewInit();

        glGenVertexArrays(1,&vao);
        glBindVertexArray(vao);

        quadVbo = GrabVbo();
        glBindBuffer(GL_ARRAY_BUFFER,quadVbo);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(0, 3, GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)0);
        glVertexAttribPointer(1, 2, GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)(3*sizeof(float)));
        glBufferData(GL_ARRAY_BUFFER,sizeof(quadVerts),quadVerts,GL_STATIC_DRAW);
        sceneRoot = new GameObject();
    }
}

void DreamTerminate()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

void UseOrtho()
{
    glOrtho(0,screenWidth,screenHeight,0,0.0,100.0f);
}

QuadDrawCall::QuadDrawCall(glm::vec2 position, glm::vec2 size, std::string texture, glm::vec4 color)
{
    this->position = position;
    this->size = size;
    this->texture = texture;
}




void QuadDrawCall::draw()
{
    std::cout << "Drawing Quad: \n";
    GetShader("quad")->bind();

    std::cout << this->texture;
    if(this->texture != "")
    {
        Texture* tex = GetTexture("test.png");
        std::cout << tex->textureHandle;
        glBindTexture(GL_TEXTURE_2D, tex->textureHandle);
    }


    glBindBuffer(GL_ARRAY_BUFFER,quadVbo);
    glDrawArrays(GL_TRIANGLES,0,6);
}

void DreamDrawQuadSolid(int x, int y, int w, int h, glm::vec4 color)
{
    QuadDrawCall* call = new QuadDrawCall(glm::vec2(x,y),glm::vec2(w,h),"",color);
    renderQueue.push_back(call);
}

void DreamDrawQuadTexture(int x, int y, int w, int h, glm::vec4 color, std::string texture)
{
    QuadDrawCall* call = new QuadDrawCall(glm::vec2(x,y), glm::vec2(w,h),texture,color);
    renderQueue.push_back(call);
}


void GameObject::processTick(float delta)
{
    tick(delta);
    postTick(delta);
}

void GameObject::postTick(float delta)
{
    for(GameObject* child : children)
    {
        child->processTick(delta);
    }
}

void GameObject::processRender()
{
    render();
    postRender();
}

void GameObject::postRender()
{
    for(GameObject* child : children)
    {
        child->processRender();
    }
}

void GameObject::start(){}
void GameObject::tick(float delta){}
void GameObject::render(){}
void GameObject::destroy(){}

//=======OPENGL UTILITIES======================
unsigned int GrabVbo()
{
    unsigned int vbo;
    glGenBuffers(1, &vbo);
    return vbo;
}
