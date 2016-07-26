//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/simpleLightingShader.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/unitTestGLDrawing.h"
#include "pxr/imaging/hd/unitTestHelper.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec3d.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>

class My_TestGLDrawing : public Hd_UnitTestGLDrawing {
public:
    My_TestGLDrawing() {
        _reprName = HdTokens->hull;
        _refineLevel = 0;
        _cullStyle = HdCullStyleNothing;
        _testLighting = false;

        SetCameraRotate(60.0f, 0.0f);
        SetCameraTranslate(GfVec3f(0, 0, -20.0f-1.7320508f*2.0f));
    }

    // Hd_UnitTestGLDrawing overrides
    virtual void InitTest();
    virtual void DrawTest();
    virtual void OffscreenTest();

protected:
    virtual void ParseArgs(int argc, char *argv[]);

private:
    Hd_TestDriver* _driver;
    HdSimpleLightingShaderSharedPtr _lightingShader;
    std::vector<GfVec4d> _clipPlanes;

    TfToken _reprName;
    int _refineLevel;
    HdCullStyle _cullStyle;
    bool _testLighting;
    std::string _outputFilePath;
};

////////////////////////////////////////////////////////////

GLuint vao;

void
My_TestGLDrawing::InitTest()
{
    std::cout << "My_TestGLDrawing::InitTest() " << _reprName << "\n";

    _driver = new Hd_TestDriver(_reprName);
    Hd_UnitTestDelegate &delegate = _driver->GetDelegate();
    delegate.SetRefineLevel(_refineLevel);

    GfVec3f center(0);

    delegate.PopulateInvalidPrimsSet();
    center = delegate.PopulateBasicTestSet();

    // center camera
    SetCameraTranslate(GetCameraTranslate() - center);

    // XXX: Setup a VAO, the current drawing engine will not yet do this.
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindVertexArray(0);

    if (_testLighting) {
        _lightingShader.reset(new HdSimpleLightingShader());
        _driver->GetRenderPassState()->SetLightingShader(
            _lightingShader);

        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);

        _lightingShader->SetLightingStateFromOpenGL();
    }

    _driver->GetRenderPassState()->SetClipPlanes(_clipPlanes);
}

void
My_TestGLDrawing::DrawTest()
{
    GLfloat clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    glClearBufferfv(GL_COLOR, 0, clearColor);

    GLfloat clearDepth[1] = { 1.0f };
    glClearBufferfv(GL_DEPTH, 0, clearDepth);

    int width = GetWidth(), height = GetHeight();
    GfMatrix4d viewMatrix = GetViewMatrix();
    GfMatrix4d projMatrix = GetProjectionMatrix();

    _driver->SetCullStyle(_cullStyle);

    // camera
    _driver->SetCamera(viewMatrix, projMatrix, GfVec4d(0, 0, width, height));

    glViewport(0, 0, width, height);

    glEnable(GL_DEPTH_TEST);

    glBindVertexArray(vao);

    _driver->Draw();

    glBindVertexArray(0);
}

void
My_TestGLDrawing::OffscreenTest()
{
    DrawTest();

    if (not _outputFilePath.empty()) {
        WriteToFile("color", _outputFilePath);
    }
}

/* virtual */
void
My_TestGLDrawing::ParseArgs(int argc, char *argv[])
{
    for (int i=0; i<argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--repr") {
            _reprName = TfToken(argv[++i]);
        } else if (arg == "--refineLevel") {
            _refineLevel = atoi(argv[++i]);
        } else if (std::string(argv[i]) == "--cullStyle") {
            std::string style = argv[++i];
            if (style == "Nothing") {
                _cullStyle = HdCullStyleNothing;
            } else if (style == "Back") {
                _cullStyle = HdCullStyleBack;
            } else if (style == "Front") {
                _cullStyle = HdCullStyleFront;
            } else if (style == "BackUnlessDoubleSided") {
                _cullStyle = HdCullStyleBackUnlessDoubleSided;
            } else if (style == "FrontUnlessDoubleSided") {
                _cullStyle = HdCullStyleFrontUnlessDoubleSided;
            } else {
                std::cerr << "Error: Unknown cullstyle = " << style << "\n";
                exit(EXIT_FAILURE);
            }
        } else if (arg == "--lighting") {
            _testLighting = true;
        } else if (arg == "--clipPlane" and i+4<argc) {
            GfVec4d clipPlane;
            clipPlane[0] = std::strtod(argv[++i], NULL);
            clipPlane[1] = std::strtod(argv[++i], NULL);
            clipPlane[2] = std::strtod(argv[++i], NULL);
            clipPlane[3] = std::strtod(argv[++i], NULL);
            _clipPlanes.push_back(clipPlane);
        } else if (arg == "--write") {
            _outputFilePath = argv[++i];
        }
    }
}

void
BasicTest(int argc, char *argv[])
{
    My_TestGLDrawing driver;
    driver.RunTest(argc, argv);
}

int main(int argc, char *argv[])
{
    TfErrorMark mark;

    BasicTest(argc, argv);

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
