/**********************************************************************************
// Multi (Código Fonte)
//
// Criação:     27 Abr 2016
// Atualização: 15 Set 2023
// Compilador:  Visual C++ 2022
//
// Descrição:   Constrói cena usando vários buffers, um por objeto
//
**********************************************************************************/

#include "DXUT.h"
#include "string"
#include <fstream>
#include <sstream>


// ------------------------------------------------------------------------------

struct ObjData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    size_t VertexCount() const { return vertices.size(); }
    size_t IndexCount() const { return indices.size(); }
    Vertex* VertexData() { return vertices.data(); }
    uint32_t* IndexData() { return indices.data(); }
};

struct ObjectConstants
{
    XMFLOAT4X4 WorldViewProj =
    { 1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f };

    XMFLOAT4 objColor;
};

// ------------------------------------------------------------------------------

class Multi : public App
{
private:
    ID3D12RootSignature* rootSignature = nullptr;
    ID3D12PipelineState* pipelineState = nullptr;
    vector<Object> scene;
    Object* selectedObj;
    int selectedIndex = -1;

    Geometry quad;
    Geometry box;
    Geometry cylinder;
    Geometry sphere;
    Geometry geoSphere;
    Geometry grid;

    vector<Object> linhas;

    Timer timer;
    bool spinning = true;
    bool changeTranslation = true;

    XMFLOAT4X4 Identity = {};
    XMFLOAT4X4 View = {};
    XMFLOAT4X4 Proj = {};

    XMFLOAT4X4 ViewOrtFront = {};
    XMFLOAT4X4 ProjOrtFront = {};

    XMFLOAT4X4 ViewOrtSide = {};
    XMFLOAT4X4 ProjOrtSide = {};

    XMFLOAT4X4 ViewOrtTop = {};
    XMFLOAT4X4 ProjOrtTop = {};

    XMFLOAT4X4 ViewOrtTotal = {};
    XMFLOAT4X4 ViewOrtTotalSide = {};
    XMFLOAT4X4 ProjOrtTotal = {};

    //XMFLOAT4X4 ViewAtual = {};
    //XMFLOAT4X4 ProjAtual = {};

    float theta = 0;
    float phi = 0;
    float radius = 0;
    float lastMousePosX = 0;
    float lastMousePosY = 0;


    D3D12_VIEWPORT viewPortTotal;
    D3D12_VIEWPORT viewPortPers;
    D3D12_VIEWPORT viewPortOrtFront;
    D3D12_VIEWPORT viewPortOrtSide;
    D3D12_VIEWPORT viewPortOrtTop;
    bool quadView = false;

public:
    ObjData LoadOBJ(const std::string& filename);
    void Init();
    void Update();
    void DrawObjects(int);
    void DrawLines();
    void Draw();
    void Finalize();

    void BuildRootSignature();
    void BuildPipelineState();
};

// ------------------------------------------------------------------------------
ObjData Multi::LoadOBJ(const std::string& filename) {
    ObjData objData;
    std::ifstream file(filename);
    std::string line;

    std::vector<XMFLOAT3> positions;
    std::vector<XMFLOAT3> normals;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            // Vértices (posições)
            XMFLOAT3 position;
            iss >> position.x >> position.y >> position.z;
            positions.push_back(position);
        }
        else if (prefix == "vn") {
            // Normais
            XMFLOAT3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }
        else if (prefix == "f") {
            // Faces (índices de vértices, coordenadas de textura e normais)
            std::vector<uint32_t> vertexIndices;
            std::vector<uint32_t> normalIndices;

            std::string faceStr;
            std::getline(iss, faceStr);
            std::istringstream faceStream(faceStr);

            std::string vertexData;
            while (faceStream >> vertexData) {
                // Substituir '/' por espaços manualmente
                for (char& c : vertexData) {
                    if (c == '/') {
                        c = ' ';
                    }
                }

                std::istringstream vertexStream(vertexData);
                uint32_t vIdx, nIdx;

                vertexStream >> vIdx;  // Índice de vértice
                if (vertexStream.peek() == ' ') vertexStream.ignore(); // Pula se houver coordenadas de textura
                vertexStream >> nIdx;  // Índice de normal

                // Convertendo de 1-based para 0-based
                vertexIndices.push_back(vIdx - 1);
                normalIndices.push_back(nIdx - 1);
            }

            // Se a face tem mais de 3 vértices, dividimos em triângulos
            for (size_t i = 1; i < vertexIndices.size() - 1; ++i) {
                objData.indices.push_back(vertexIndices[0]);
                objData.indices.push_back(vertexIndices[i]);
                objData.indices.push_back(vertexIndices[i + 1]);
            }
        }
    }

    // Adiciona os vértices ao ObjData
    for (auto& pos : positions) {
        Vertex vertex;
        vertex.pos = pos;
        vertex.color = XMFLOAT4(DirectX::Colors::DimGray); // Definir cor padrão
        objData.vertices.push_back(vertex);
    }

    return objData;
}


void Multi::Init()
{
    graphics->ResetCommands();




    //vbSize_linhas, sizeof(Vertex);
    //meshLinhas->IndexBuffer(linhas, vbSize_linhas);

    // -----------------------------
    // Viewports
    // -----------------------------
    
    // viewport total
    viewPortTotal.TopLeftX = 0.0f;
    viewPortTotal.TopLeftY = 0.0f;
    viewPortTotal.Width = float(window->Width());
    viewPortTotal.Height = float(window->Height());
    viewPortTotal.MinDepth = 0.0f;
    viewPortTotal.MaxDepth = 1.0f;

    // viewport perspectiva
    viewPortPers.TopLeftX = float(window->Width() / 2);
    viewPortPers.TopLeftY = float(window->Height() / 2);
    viewPortPers.Width = float(window->Width() / 2);
    viewPortPers.Height = float(window->Height() / 2);
    viewPortPers.MinDepth = 0.0f;
    viewPortPers.MaxDepth = 1.0f;

    // viewport ortogonal frontal
    viewPortOrtFront.TopLeftX = 0.0f;
    viewPortOrtFront.TopLeftY = 0.0f;
    viewPortOrtFront.Width = float(window->Width() / 2);
    viewPortOrtFront.Height = float(window->Height() / 2);
    viewPortOrtFront.MinDepth = 0.0f;
    viewPortOrtFront.MaxDepth = 1.0f;

    // viewport ortogonal lateral
    viewPortOrtSide.TopLeftX = 0.0f;
    viewPortOrtSide.TopLeftY = float(window->Height() / 2);
    viewPortOrtSide.Width = float(window->Width() / 2);
    viewPortOrtSide.Height = float(window->Height() / 2);
    viewPortOrtSide.MinDepth = 0.0f;
    viewPortOrtSide.MaxDepth = 1.0f;

    // viewport ortogonal superior
    viewPortOrtTop.TopLeftX = float(window->Width() / 2);
    viewPortOrtTop.TopLeftY = 0.0f;
    viewPortOrtTop.Width = float(window->Width() / 2);
    viewPortOrtTop.Height = float(window->Height() / 2);
    viewPortOrtTop.MinDepth = 0.0f;
    viewPortOrtTop.MaxDepth = 1.0f;

    
    // Exemplo para a vista ortogonal frontal:
    XMStoreFloat4x4(&ViewOrtFront, XMMatrixLookAtLH(
        XMVectorSet(0.0f, 0.01f, 10.0f, 1.0f), // Posição da câmera
        XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),  // Para onde a câmera olha
        XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)   // Para cima
    ));

    // Exemplo para a vista ortogonal lateral:
    XMStoreFloat4x4(&ViewOrtSide, XMMatrixLookAtLH(
        XMVectorSet(-10.0f, 0.01f, 0.0f, 1.0f), // Posição da câmera
        XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),  // Para onde a câmera olha
        XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)   // Para cima
    ));

    // Exemplo para a vista ortogonal superior:
    XMStoreFloat4x4(&ViewOrtTop, XMMatrixLookAtLH(
        XMVectorSet(0.0f, 10.0f, 0.0f, 1.0f),  // Posição da câmera
        XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),  // Para onde a câmera olha
        XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f)  // Para cima
    ));

    XMStoreFloat4x4(&ViewOrtTotal, XMMatrixLookAtLH(
        XMVectorSet(30.0f, 0.01f, 30.0f, 1.0f), // Posição da câmera
        XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),  // Para onde a câmera olha
        XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
    ));

    XMStoreFloat4x4(&ViewOrtTotalSide, XMMatrixLookAtLH(
        XMVectorSet(30.0f, 0.01f, 0.01f, 1.0f), // Posição da câmera
        XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),  // Para onde a câmera olha
        XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f)
    ));

    // -----------------------------
    // Parâmetros Iniciais da Câmera
    // -----------------------------
 
    // controla rotação da câmera
    theta = XM_PIDIV4;
    phi = 1.3f;
    radius = 5.0f;

    // pega última posição do mouse
    lastMousePosX = (float) input->MouseX();
    lastMousePosY = (float) input->MouseY();

    // inicializa as matrizes Identity e View para a identidade
    Identity = View = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f };

    // inicializa a matriz de projeção
    XMStoreFloat4x4(&Proj, XMMatrixPerspectiveFovLH(
        XMConvertToRadians(45.0f), 
        window->AspectRatio(), 
        1.0f, 100.0f));

    //XMStoreFloat4x4(&ProjAtual, XMMatrixPerspectiveFovLH(
        //XMConvertToRadians(45.0f),
        //window->AspectRatio(),
        //1.0f, 100.0f));


    XMStoreFloat4x4(&ProjOrtFront, XMMatrixOrthographicLH(float(window->Width() /2 / 75), float(window->Height() / 2 / 75), 1.0f, 100.0f));
    XMStoreFloat4x4(&ProjOrtSide, XMMatrixOrthographicLH(float(window->Width() /2 / 75), float(window->Height() / 2 / 75), 1.0f, 100.0f));
    XMStoreFloat4x4(&ProjOrtTop, XMMatrixOrthographicLH(float(window->Width() /2 / 75), float(window->Height() / 2 / 75), 1.0f, 100.0f));
    XMStoreFloat4x4(&ProjOrtTotal, XMMatrixOrthographicLH(float(window->Width() / 3 / 75), float(window->Height() /3  / 75), 1.0f, 100.0f));
    /*XMStoreFloat4x4(&ProjOrtTotal, XMMatrixPerspectiveFovLH(
        XMConvertToRadians(45.0f),
        window->AspectRatio(),
        1.0f, 100.0f));*/

   /* XMStoreFloat4x4(&ProjOrtFront, XMMatrixOrthographicLH(5.0f, 5.0f, 1.0f, 100.0f));
    XMStoreFloat4x4(&ProjOrtSide, XMMatrixOrthographicLH(5.0f, 5.0f, 1.0f, 100.0f));
    XMStoreFloat4x4(&ProjOrtTop, XMMatrixOrthographicLH(5.0f, 5.0f, 1.0f, 100.0f));*/

    // ----------------------------------------
    // Criação da Geometria: Vértices e Índices
    // ----------------------------------------

    quad =Quad(2.0f, 2.0f);
    box = Box(2.0f, 2.0f, 2.0f);
    cylinder = Cylinder(1.0f, 1.0f, 3.0f, 20, 20);
    sphere = Sphere(1.0f, 20, 20);
    geoSphere = GeoSphere(1.0f, 3);
    grid = Grid(3.0f, 3.0f, 20, 20);

    for (auto& v : quad.vertices) v.color = XMFLOAT4(DirectX::Colors::DimGray);
    for (auto& v : box.vertices) v.color = XMFLOAT4(DirectX::Colors::DimGray);
    for (auto& v : cylinder.vertices) v.color = XMFLOAT4(DirectX::Colors::DimGray);
    for (auto& v : sphere.vertices) v.color = XMFLOAT4(DirectX::Colors::DimGray);
    for (auto& v : geoSphere.vertices) v.color = XMFLOAT4(DirectX::Colors::DimGray);
    for (auto& v : grid.vertices) v.color = XMFLOAT4(DirectX::Colors::DimGray);

    // ---------------------------------------------------------------
    // Alocação e Cópia de Vertex, Index e Constant Buffers para a GPU
    // ---------------------------------------------------------------

    // grid
    Object gridObj;
    gridObj.mesh = new Mesh();
    gridObj.world = Identity;
    gridObj.mesh->VertexBuffer(grid.VertexData(), grid.VertexCount() * sizeof(Vertex), sizeof(Vertex));
    gridObj.mesh->IndexBuffer(grid.IndexData(), grid.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
    gridObj.mesh->ConstantBuffer(sizeof(ObjectConstants), 4);
    gridObj.submesh.indexCount = grid.IndexCount();
    scene.push_back(gridObj);
    selectedIndex = (selectedIndex + 1) % scene.size();
    

    Object gridObjL0;
    XMStoreFloat4x4(&gridObjL0.world,
        XMMatrixScaling(1.0f, 1.0f, 1.0f) *
        XMMatrixRotationZ(0.2f) *
        XMMatrixTranslation(0.0f, 0.1f, 0.0f)
    );

    gridObjL0.mesh = new Mesh();
    gridObjL0.world = Identity;
    gridObjL0.mesh->VertexBuffer(grid.VertexData(), grid.VertexCount() * sizeof(Vertex), sizeof(Vertex));
    gridObjL0.mesh->IndexBuffer(grid.IndexData(), grid.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
    gridObjL0.mesh->ConstantBuffer(sizeof(ObjectConstants), 4);
    gridObjL0.submesh.indexCount = grid.IndexCount();
    linhas.push_back(gridObjL0);

    Object gridObjL1;
    XMStoreFloat4x4(&gridObjL1.world,
            XMMatrixScaling(1.0f, 1.0f, 1.0f) *
            XMMatrixRotationZ(0.2f) *
            XMMatrixTranslation(0.0f, 0.1f, 0.0f)
    );

    gridObjL1.mesh = new Mesh();
    gridObjL1.world = Identity;
    gridObjL1.mesh->VertexBuffer(grid.VertexData(), grid.VertexCount() * sizeof(Vertex), sizeof(Vertex));
    gridObjL1.mesh->IndexBuffer(grid.IndexData(), grid.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
    gridObjL1.mesh->ConstantBuffer(sizeof(ObjectConstants), 4);
    gridObjL1.submesh.indexCount = grid.IndexCount();
    linhas.push_back(gridObjL1);

    /*XMMATRIX rotation = XMMatrixRotationZ(0.03f);

    XMMATRIX currentWorldL0 = XMLoadFloat4x4(&selectedObj->world);
    Object* l0 = &gridObjL0;
    XMStoreFloat4x4(&l0->world, rotation * currentWorldL0);

    XMMATRIX currentWorldL1 = XMLoadFloat4x4(&selectedObj->world);
    Object* l1 = &gridObjL1;
    XMStoreFloat4x4(&l1->world, rotation *currentWorldL1);*/

    // ---------------------------------------

    BuildRootSignature();
    BuildPipelineState();    

    // ---------------------------------------
    graphics->SubmitCommands();

    timer.Start();
}

// ------------------------------------------------------------------------------

void Multi::Update()
{
    // sai com o pressionamento da tecla ESC
    if (input->KeyPress(VK_ESCAPE))
        window->Close();

    // ativa ou desativa a quad view
    if (input->KeyPress('V'))
    {
        quadView = !quadView;
    }
    if (input->KeyPress('R'))
    {
        changeTranslation = !changeTranslation;
    }
    if (input->KeyPress(VK_DELETE))
    {
        if (selectedIndex >= 0 && selectedIndex < scene.size())
        {
            delete scene[selectedIndex].mesh;
            scene.erase(scene.begin() + selectedIndex);

            if (!scene.empty())
            {
                selectedIndex = selectedIndex % scene.size();
                selectedObj = &scene[selectedIndex];
            }
            else
            {
                selectedIndex = -1;
                selectedObj = nullptr;
            }
        }
    }
    graphics->ResetCommands();

    if (input->KeyPress('Q')) {

        Object quadObj;
        XMStoreFloat4x4(&quadObj.world,
            XMMatrixScaling(0.5f, 0.5f, 0.5f) *
            XMMatrixTranslation(0.0f, 0.5f, 0.0f));

        quadObj.mesh = new Mesh();
        quadObj.mesh->VertexBuffer(quad.VertexData(), quad.VertexCount() * sizeof(Vertex), sizeof(Vertex));
        quadObj.mesh->IndexBuffer(quad.IndexData(), quad.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
        quadObj.mesh->ConstantBuffer(sizeof(ObjectConstants), 4);
        quadObj.submesh.indexCount = quad.IndexCount();
        scene.push_back(quadObj);

        selectedIndex = scene.size() - 1;
        selectedObj = &scene.back();

    }

    if (input->KeyPress('B')) {
        // box
        Object boxObj;
        XMStoreFloat4x4(&boxObj.world,
            XMMatrixScaling(0.5f, 0.5f, 0.5f) *
            XMMatrixTranslation(0.0f, 0.5f, 0.0f));

        boxObj.mesh = new Mesh();
        boxObj.mesh->VertexBuffer(box.VertexData(), box.VertexCount() * sizeof(Vertex), sizeof(Vertex));
        boxObj.mesh->IndexBuffer(box.IndexData(), box.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
        boxObj.mesh->ConstantBuffer(sizeof(ObjectConstants), 4);
        boxObj.submesh.indexCount = box.IndexCount();
        scene.push_back(boxObj);

        selectedIndex = scene.size() - 1;
        selectedObj = &scene.back();

    }

    if (input->KeyPress('C')) {
        // cylinder
        Object cylinderObj;
        XMStoreFloat4x4(&cylinderObj.world,
            XMMatrixScaling(0.5f, 0.5f, 0.5f) *
            XMMatrixTranslation(0.0f, 0.75f, 0.0f));

        cylinderObj.mesh = new Mesh();
        cylinderObj.mesh->VertexBuffer(cylinder.VertexData(), cylinder.VertexCount() * sizeof(Vertex), sizeof(Vertex));
        cylinderObj.mesh->IndexBuffer(cylinder.IndexData(), cylinder.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
        cylinderObj.mesh->ConstantBuffer(sizeof(ObjectConstants), 4);
        cylinderObj.submesh.indexCount = cylinder.IndexCount();
        scene.push_back(cylinderObj);

        selectedIndex = scene.size() - 1;
        selectedObj = &scene.back();

    }

    if (input->KeyPress('S')) {
        // sphere
        Object sphereObj;
        XMStoreFloat4x4(&sphereObj.world,
            XMMatrixScaling(0.5f, 0.5f, 0.5f) *
            XMMatrixTranslation(0.0f, 0.5f, 0.0f));

        sphereObj.mesh = new Mesh();
        sphereObj.mesh->VertexBuffer(sphere.VertexData(), sphere.VertexCount() * sizeof(Vertex), sizeof(Vertex));
        sphereObj.mesh->IndexBuffer(sphere.IndexData(), sphere.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
        sphereObj.mesh->ConstantBuffer(sizeof(ObjectConstants), 4);
        sphereObj.submesh.indexCount = sphere.IndexCount();
        scene.push_back(sphereObj);

        selectedIndex = scene.size() - 1;
        selectedObj = &scene.back();

    }

    if (input->KeyPress('G')) {
        // geo sphere
        Object geoSphereObj;
        XMStoreFloat4x4(&geoSphereObj.world,
            XMMatrixScaling(0.5f, 0.5f, 0.5f) *
            XMMatrixTranslation(0.0f, 0.5f, 0.0f));

        geoSphereObj.mesh = new Mesh();
        geoSphereObj.mesh->VertexBuffer(geoSphere.VertexData(), geoSphere.VertexCount() * sizeof(Vertex), sizeof(Vertex));
        geoSphereObj.mesh->IndexBuffer(geoSphere.IndexData(), geoSphere.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
        geoSphereObj.mesh->ConstantBuffer(sizeof(ObjectConstants), 4);
        geoSphereObj.submesh.indexCount = geoSphere.IndexCount();
        scene.push_back(geoSphereObj);

        selectedIndex = scene.size() - 1;
        selectedObj = &scene.back();

    }

    if (input->KeyPress('P')) {
        // grid
        Object gridObj;
        gridObj.mesh = new Mesh();
        gridObj.world = Identity;
        gridObj.mesh->VertexBuffer(grid.VertexData(), grid.VertexCount() * sizeof(Vertex), sizeof(Vertex));
        gridObj.mesh->IndexBuffer(grid.IndexData(), grid.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
        gridObj.mesh->ConstantBuffer(sizeof(ObjectConstants), 4);
        gridObj.submesh.indexCount = grid.IndexCount();
        scene.push_back(gridObj);

        selectedIndex = scene.size() - 1;
        selectedObj = &scene.back();

    }

    if (input->KeyPress('1')) {
        // Carregar o arquivo .obj
        ObjData ballData = LoadOBJ("ball.obj");

        Object ballDataObj;
        ballDataObj.mesh = new Mesh();
        ballDataObj.world = Identity;
        ballDataObj.mesh->VertexBuffer(ballData.VertexData(), ballData.VertexCount() * sizeof(Vertex), sizeof(Vertex));
        ballDataObj.mesh->IndexBuffer(ballData.IndexData(), ballData.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
        ballDataObj.mesh->ConstantBuffer(sizeof(ObjectConstants), 4);
        ballDataObj.submesh.indexCount = ballData.IndexCount();
        scene.push_back(ballDataObj);

        selectedIndex = scene.size() - 1;
        selectedObj = &scene.back(); 
    }
    if (input->KeyPress('2')) {
        // Carregar o arquivo .obj
        ObjData capsuleData = LoadOBJ("capsule.obj");

        Object capsuleDataObj;
        capsuleDataObj.mesh = new Mesh();
        capsuleDataObj.world = Identity;
        capsuleDataObj.mesh->VertexBuffer(capsuleData.VertexData(), capsuleData.VertexCount() * sizeof(Vertex), sizeof(Vertex));
        capsuleDataObj.mesh->IndexBuffer(capsuleData.IndexData(), capsuleData.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
        capsuleDataObj.mesh->ConstantBuffer(sizeof(ObjectConstants), 4);
        capsuleDataObj.submesh.indexCount = capsuleData.IndexCount();
        scene.push_back(capsuleDataObj);

        selectedIndex = scene.size() - 1;
        selectedObj = &scene.back();
    }
    if (input->KeyPress('3')) {
        // Carregar o arquivo .obj
        ObjData houseData = LoadOBJ("house.obj");

        Object houseDataObj;
        houseDataObj.mesh = new Mesh();
        houseDataObj.world = Identity;
        houseDataObj.mesh->VertexBuffer(houseData.VertexData(), houseData.VertexCount() * sizeof(Vertex), sizeof(Vertex));
        houseDataObj.mesh->IndexBuffer(houseData.IndexData(), houseData.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
        houseDataObj.mesh->ConstantBuffer(sizeof(ObjectConstants), 4);
        houseDataObj.submesh.indexCount = houseData.IndexCount();
        scene.push_back(houseDataObj);

        selectedIndex = scene.size() - 1;
        selectedObj = &scene.back();
    }
    if (input->KeyPress('4')) {
        // Carregar o arquivo .obj
        ObjData monkeyData = LoadOBJ("monkey.obj");

        Object monkeyDataObj;
        monkeyDataObj.mesh = new Mesh();
        monkeyDataObj.world = Identity;
        monkeyDataObj.mesh->VertexBuffer(monkeyData.VertexData(), monkeyData.VertexCount() * sizeof(Vertex), sizeof(Vertex));
        monkeyDataObj.mesh->IndexBuffer(monkeyData.IndexData(), monkeyData.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
        monkeyDataObj.mesh->ConstantBuffer(sizeof(ObjectConstants), 4);
        monkeyDataObj.submesh.indexCount = monkeyData.IndexCount();
        scene.push_back(monkeyDataObj);

        selectedIndex = scene.size() - 1;
        selectedObj = &scene.back();
    }
    if (input->KeyPress('5')) {
        // Carregar o arquivo .obj
        ObjData thorusData = LoadOBJ("thorus.obj");

        Object thorusDataObj;
        thorusDataObj.mesh = new Mesh();
        thorusDataObj.world = Identity;
        thorusDataObj.mesh->VertexBuffer(thorusData.VertexData(), thorusData.VertexCount() * sizeof(Vertex), sizeof(Vertex));
        thorusDataObj.mesh->IndexBuffer(thorusData.IndexData(), thorusData.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
        thorusDataObj.mesh->ConstantBuffer(sizeof(ObjectConstants), 4);
        thorusDataObj.submesh.indexCount = thorusData.IndexCount();
        scene.push_back(thorusDataObj);

        selectedIndex = scene.size() - 1;
        selectedObj = &scene.back();
    }
    if (input->KeyPress('6')) {
        // Carregar o arquivo .obj
        ObjData data = LoadOBJ("plane.obj");

        Object dataObj;
        dataObj.mesh = new Mesh();
        dataObj.world = Identity;
        dataObj.mesh->VertexBuffer(data.VertexData(), data.VertexCount() * sizeof(Vertex), sizeof(Vertex));
        dataObj.mesh->IndexBuffer(data.IndexData(), data.IndexCount() * sizeof(uint), DXGI_FORMAT_R32_UINT);
        dataObj.mesh->ConstantBuffer(sizeof(ObjectConstants), 4);
        dataObj.submesh.indexCount = data.IndexCount();
        scene.push_back(dataObj);

        selectedIndex = scene.size() - 1;
        selectedObj = &scene.back();
    }

    float mousePosX = (float)input->MouseX();
    float mousePosY = (float)input->MouseY();
    
    if (input->KeyPress(VK_TAB)) {
        if (scene.empty()) return;
        selectedIndex = (selectedIndex + 1) % scene.size();
        selectedObj = &scene[selectedIndex];
    }

    if (input->KeyDown(VK_LBUTTON))
    {
        // cada pixel corresponde a 1/4 de grau
        float dx = XMConvertToRadians(0.25f * (mousePosX - lastMousePosX));
        float dy = XMConvertToRadians(0.25f * (mousePosY - lastMousePosY));

        // atualiza ângulos com base no deslocamento do mouse 
        // para orbitar a câmera ao redor da caixa
        theta += dx;
        phi += dy;

        // restringe o ângulo de phi ]0-180[ graus
        phi = phi < 0.1f ? 0.1f : (phi > (XM_PI - 0.1f) ? XM_PI - 0.1f : phi);
    }
    else if (input->KeyDown(VK_RBUTTON))
    {
        // cada pixel corresponde a 0.05 unidades
        float dx = 0.05f * (mousePosX - lastMousePosX);
        float dy = 0.05f * (mousePosY - lastMousePosY);

        // atualiza o raio da câmera com base no deslocamento do mouse 
        radius += dx - dy;

        // restringe o raio (3 a 15 unidades)
        radius = radius < 3.0f ? 3.0f : (radius > 15.0f ? 15.0f : radius);
    }

    //XMStoreFloat4x4(&ProjOrtFront, XMMatrixOrthographicLH(float(window->Width() / 2 * radius / 500), float(window->Height() / 2 * radius / 500), 1.0f, 100.0f));
    //XMStoreFloat4x4(&ProjOrtSide, XMMatrixOrthographicLH(float(window->Width() / 2 * radius / 500), float(window->Height() / 2 * radius / 500), 1.0f, 100.0f));
    //XMStoreFloat4x4(&ProjOrtTop, XMMatrixOrthographicLH(float(window->Width() / 2 * radius / 500), float(window->Height() / 2 * radius / 500), 1.0f, 100.0f));

    lastMousePosX = mousePosX;
    lastMousePosY = mousePosY;

    // converte coordenadas esféricas para cartesianas
    float x = radius * sinf(phi) * cosf(theta);
    float z = radius * sinf(phi) * sinf(theta);
    float y = radius * cosf(phi);

    // Variáveis para controlar o movimento da câmera
    float targetMoveSpeed = 0.1f;  // Velocidade de movimento do target
    static XMFLOAT3 targetPos(0.0f, 0.0f, 0.0f);  // Posição do target da câmera (inicialmente em (0,0,0))

    // Controle da posição do target da câmera com base no input
    if (input->KeyDown('J')) {
        targetPos.x -= targetMoveSpeed;  // Move para a esquerda
    }
    if (input->KeyDown('L')) {
        targetPos.x += targetMoveSpeed;  // Move para a direita
    }
    if (input->KeyDown('I')) {
        targetPos.z -= targetMoveSpeed;  // Move para frente
    }
    if (input->KeyDown('K')) {
        targetPos.z += targetMoveSpeed;  // Move para trás
    }
    if (input->KeyDown('Y')) {
        targetPos.y += targetMoveSpeed;  // Sobe no eixo Y
    }
    if (input->KeyDown('H')) {
        targetPos.y -= targetMoveSpeed;  // Desce no eixo Y
    }

    // constrói a matriz da câmera (view matrix)
    XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
    XMVECTOR target = XMVectorSet(targetPos.x, targetPos.y, targetPos.z, 1.0f);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&View, view);

    // carrega matriz de projeção em uma XMMATRIX
    XMMATRIX proj = XMLoadFloat4x4(&Proj);

    // modifica matriz de mundo do objeto selecionado
    if (selectedIndex != -1) {
        if (selectedObj != nullptr) {

            // Carrega a matriz de mundo atual do objeto
            XMMATRIX currentWorld = XMLoadFloat4x4(&selectedObj->world);

            XMVECTOR scale, rotation, translation;
            XMMatrixDecompose(&scale, &rotation, &translation, currentWorld);

            // para girar o objeto (opcional)
            //currentWorld *= XMMatrixRotationY(float(timer.Elapsed())); 
            
            if (changeTranslation) {
                // Move no eixo X com setas esquerda/direita
                if (input->KeyDown(VK_LEFT)) {
                    currentWorld *= XMMatrixTranslation(0.05f, 0.0f, 0.0f);
                }
                if (input->KeyDown(VK_RIGHT)) {
                    currentWorld *= XMMatrixTranslation(-0.05f, 0.0f, 0.0f);
                }
                // Move no eixo Z com setas cima/baixo
                if (input->KeyDown(VK_UP)) {
                    currentWorld *= XMMatrixTranslation(0.0f, 0.0f, -0.05f);
                }
                if (input->KeyDown(VK_DOWN)) {
                    currentWorld *= XMMatrixTranslation(0.0f, 0.0f, 0.05f);
                }
                // Move no eixo Y com Ctrl (descer) e Shift (subir)
                if (input->KeyDown(VK_SHIFT)) {
                    currentWorld *= XMMatrixTranslation(0.0f, 0.05f, 0.0f);
                }
                if (input->KeyDown(VK_CONTROL)) {
                    currentWorld *= XMMatrixTranslation(0.0f, -0.05f, 0.0f);
                }
            }
            // escala
            if (input->KeyDown('X')) {
                currentWorld = XMMatrixScaling(1.01f, 1.01f, 1.01f) * currentWorld;
            }
            if (input->KeyDown('Z')) {
                currentWorld = XMMatrixScaling(0.99f, 0.99f, 0.99f) * currentWorld;
            }

            //  rotação
            if (!changeTranslation) {

                // Move no eixo Z
                if (input->KeyDown(VK_LEFT)) {
                    currentWorld = XMMatrixRotationZ(-0.05f) * currentWorld;
                }
                if (input->KeyDown(VK_RIGHT)) {
                    currentWorld = XMMatrixRotationZ(0.05f) * currentWorld;
                }
                // Move no eixo X com setas cima/baixo
                if (input->KeyDown(VK_UP)) {
                    currentWorld = XMMatrixRotationX(-0.05f) * currentWorld;
                }
                if (input->KeyDown(VK_DOWN)) {
                    currentWorld = XMMatrixRotationX(0.05f) * currentWorld;
                }
                // Move no eixo Y com Ctrl (Horario) e Shift (Anti-horario)
                if (input->KeyDown(VK_SHIFT)) {
                    currentWorld = XMMatrixRotationY(-0.05f) * currentWorld;
                }
                if (input->KeyDown(VK_CONTROL)) {
                    currentWorld = XMMatrixRotationY(0.05f) * currentWorld;
                }
            }


            XMStoreFloat4x4(&selectedObj->world, currentWorld);
        }

    }

    // ajusta o buffer constante de cada objeto
    XMMATRIX projOrtFront = XMLoadFloat4x4(&ProjOrtFront);
    XMMATRIX viewOrtFront = XMLoadFloat4x4(&ViewOrtFront);
    XMMATRIX projOrtSide = XMLoadFloat4x4(&ProjOrtSide);
    XMMATRIX viewOrtSide = XMLoadFloat4x4(&ViewOrtSide);
    XMMATRIX projOrtTop = XMLoadFloat4x4(&ProjOrtTop);
    XMMATRIX viewOrtTop = XMLoadFloat4x4(&ViewOrtTop);

    OutputDebugString(("Tamanho da cena: " + std::to_string(scene.size()) + "\n").c_str());
    for (auto & obj : scene)
    {   
        // carrega matriz de mundo em uma XMMATRIX
        XMMATRIX world = XMLoadFloat4x4(&obj.world);      

        // constrói matriz combinada (world x view x proj)
        XMMATRIX WorldViewProj = world * view * proj;
        XMMATRIX WorldViewProjFront = world * viewOrtFront * projOrtFront;
        XMMATRIX WorldViewProjSide = world * viewOrtSide * projOrtSide;
        XMMATRIX WorldViewProjTop = world * viewOrtTop * projOrtTop;

        bool isSelected = (&obj == selectedObj);
        XMVECTOR color = isSelected ? DirectX::Colors::Red : DirectX::Colors::DimGray;

        // atualiza o buffer constante com a matriz combinada
        ObjectConstants constants;

        XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(WorldViewProj));
        XMStoreFloat4(&constants.objColor, color);
        obj.mesh->CopyConstants(&constants, 0);

        XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(WorldViewProjFront));
        XMStoreFloat4(&constants.objColor, color);
        obj.mesh->CopyConstants(&constants, 1);

        XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(WorldViewProjSide));
        XMStoreFloat4(&constants.objColor, color);
        obj.mesh->CopyConstants(&constants, 2);

        XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(WorldViewProjTop));
        XMStoreFloat4(&constants.objColor, color);
        obj.mesh->CopyConstants(&constants, 3);
    }
    

    XMMATRIX viewOrtTotal = XMLoadFloat4x4(&ViewOrtTotal);
    XMMATRIX viewOrtTotalSide = XMLoadFloat4x4(&ViewOrtTotalSide);
    XMMATRIX projOrtTotal = XMLoadFloat4x4(&ProjOrtTotal);

    bool first = true;

    XMMATRIX worldL0 = XMLoadFloat4x4(&linhas[0].world);
    XMMATRIX worldL1 = XMLoadFloat4x4(&linhas[1].world);

    XMMATRIX WorldViewProjL0 = worldL0 * viewOrtTotal * projOrtTotal;
    XMMATRIX WorldViewProjL1 = worldL1 * viewOrtTotalSide * projOrtTotal;
    XMVECTOR color = DirectX::Colors::DimGray;
    
    ObjectConstants constants;
    XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(WorldViewProjL0));
    XMStoreFloat4(&constants.objColor, color);
    linhas[0].mesh->CopyConstants(&constants, 0);
    ObjectConstants constantsL1;
    XMStoreFloat4x4(&constantsL1.WorldViewProj, XMMatrixTranspose(WorldViewProjL1));
    XMStoreFloat4(&constantsL1.objColor, color);
    linhas[1].mesh->CopyConstants(&constantsL1, 0);


    graphics->SubmitCommands();
}

// ------------------------------------------------------------------------------

void Multi::DrawObjects(int view) {
    for (auto& obj : scene)
    {

        // comandos de configuração do pipeline
        ID3D12DescriptorHeap* descriptorHeap = obj.mesh->ConstantBufferHeap();
        graphics->CommandList()->SetDescriptorHeaps(1, &descriptorHeap);
        graphics->CommandList()->SetGraphicsRootSignature(rootSignature);
        graphics->CommandList()->IASetVertexBuffers(0, 1, obj.mesh->VertexBufferView());
        graphics->CommandList()->IASetIndexBuffer(obj.mesh->IndexBufferView());
        graphics->CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // ajusta o buffer constante associado ao vertex shader
        graphics->CommandList()->SetGraphicsRootDescriptorTable(0, obj.mesh->ConstantBufferHandle(view));

        // desenha objeto
        graphics->CommandList()->DrawIndexedInstanced(
            obj.submesh.indexCount, 1,
            obj.submesh.startIndex,
            obj.submesh.baseVertex,
            0);
    }

}


void Multi::DrawLines() {
    for (auto& obj : linhas)
    {

        // comandos de configuração do pipeline
        ID3D12DescriptorHeap* descriptorHeap = obj.mesh->ConstantBufferHeap();
        graphics->CommandList()->SetDescriptorHeaps(1, &descriptorHeap);
        graphics->CommandList()->SetGraphicsRootSignature(rootSignature);
        graphics->CommandList()->IASetVertexBuffers(0, 1, obj.mesh->VertexBufferView());
        graphics->CommandList()->IASetIndexBuffer(obj.mesh->IndexBufferView());
        graphics->CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // ajusta o buffer constante associado ao vertex shader
        graphics->CommandList()->SetGraphicsRootDescriptorTable(0, obj.mesh->ConstantBufferHandle(0));

        // desenha objeto
        graphics->CommandList()->DrawIndexedInstanced(
            obj.submesh.indexCount, 1,
            obj.submesh.startIndex,
            obj.submesh.baseVertex,
            0);
    }
}

void Multi::Draw()
{
    // limpa o backbuffer
    graphics->Clear(pipelineState);
    
    graphics->CommandList()->RSSetViewports(1, &viewPortTotal);

    if (quadView){
        DrawLines();
        for (int i = 0; i < 4; i++) {
            //XMMATRIX proj = {};
            //XMMATRIX view = {};

            switch (i) {
                case 0:
                    graphics->CommandList()->RSSetViewports(1, &viewPortPers);
                    //XMStoreFloat4x4(&ProjAtual, XMLoadFloat4x4(&Proj));
                    //XMStoreFloat4x4(&ViewAtual, XMLoadFloat4x4(&View));
                    DrawObjects(i);
                    break;
                case 1:
                    graphics->CommandList()->RSSetViewports(1, &viewPortOrtFront);
                    //XMStoreFloat4x4(&ProjAtual, XMLoadFloat4x4(&ProjOrtFront));
                    //XMStoreFloat4x4(&ViewAtual, XMLoadFloat4x4(&ViewOrtFront));
                    DrawObjects(i);
                    break;
                case 2:
                    graphics->CommandList()->RSSetViewports(1, &viewPortOrtSide);
                    //XMStoreFloat4x4(&ProjAtual, XMLoadFloat4x4(&ProjOrtSide));
                    //XMStoreFloat4x4(&ViewAtual, XMLoadFloat4x4(&ViewOrtSide));
                    DrawObjects(i);
                    break;
                case 3:
                    graphics->CommandList()->RSSetViewports(1, &viewPortOrtTop);
                    //XMStoreFloat4x4(&ProjAtual, XMLoadFloat4x4(&ProjOrtTop));
                    //XMStoreFloat4x4(&ViewAtual, XMLoadFloat4x4(&ViewOrtTop));
                    DrawObjects(i);
                    break;
            }
            
            
        }
    }


    // desenha objetos da cena
    if (!quadView) {
        for (auto& obj : scene)
        {
            // comandos de configuração do pipeline
            ID3D12DescriptorHeap* descriptorHeap = obj.mesh->ConstantBufferHeap();
            graphics->CommandList()->SetDescriptorHeaps(1, &descriptorHeap);
            graphics->CommandList()->SetGraphicsRootSignature(rootSignature);
            graphics->CommandList()->IASetVertexBuffers(0, 1, obj.mesh->VertexBufferView());
            graphics->CommandList()->IASetIndexBuffer(obj.mesh->IndexBufferView());
            graphics->CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            // ajusta o buffer constante associado ao vertex shader
            graphics->CommandList()->SetGraphicsRootDescriptorTable(0, obj.mesh->ConstantBufferHandle(0));

            // desenha objeto
            graphics->CommandList()->DrawIndexedInstanced(
                obj.submesh.indexCount, 1,
                obj.submesh.startIndex,
                obj.submesh.baseVertex,
                0);
        }
    }
    // apresenta o backbuffer na tela
    graphics->Present();    
}

// ------------------------------------------------------------------------------

void Multi::Finalize()
{
    rootSignature->Release();
    pipelineState->Release();

    for (auto& obj : scene)
        delete obj.mesh;
}


// ------------------------------------------------------------------------------
//                                     D3D                                      
// ------------------------------------------------------------------------------

void Multi::BuildRootSignature()
{
    // cria uma única tabela de descritores de CBVs
    D3D12_DESCRIPTOR_RANGE cbvTable = {};
    cbvTable.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    cbvTable.NumDescriptors = 1;
    cbvTable.BaseShaderRegister = 0;
    cbvTable.RegisterSpace = 0;
    cbvTable.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // define parâmetro raiz com uma tabela
    D3D12_ROOT_PARAMETER rootParameters[1];
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[0].DescriptorTable.pDescriptorRanges = &cbvTable;

    // uma assinatura raiz é um vetor de parâmetros raiz
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 1;
    rootSigDesc.pParameters = rootParameters;
    rootSigDesc.NumStaticSamplers = 0;
    rootSigDesc.pStaticSamplers = nullptr;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // serializa assinatura raiz
    ID3DBlob* serializedRootSig = nullptr;
    ID3DBlob* error = nullptr;

    ThrowIfFailed(D3D12SerializeRootSignature(
        &rootSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &serializedRootSig,
        &error));

    if (error != nullptr)
    {
        OutputDebugString((char*)error->GetBufferPointer());
    }

    // cria uma assinatura raiz com um único slot que aponta para  
    // uma faixa de descritores consistindo de um único buffer constante
    ThrowIfFailed(graphics->Device()->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature)));
}

// ------------------------------------------------------------------------------

void Multi::BuildPipelineState()
{
    // --------------------
    // --- Input Layout ---
    // --------------------
    
    D3D12_INPUT_ELEMENT_DESC inputLayout[2] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // --------------------
    // ----- Shaders ------
    // --------------------

    ID3DBlob* vertexShader;
    ID3DBlob* pixelShader;

    D3DReadFileToBlob(L"Shaders/Vertex.cso", &vertexShader);
    D3DReadFileToBlob(L"Shaders/Pixel.cso", &pixelShader);

    // --------------------
    // ---- Rasterizer ----
    // --------------------

    D3D12_RASTERIZER_DESC rasterizer = {};
    //rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizer.FillMode = D3D12_FILL_MODE_WIREFRAME;
    rasterizer.CullMode = D3D12_CULL_MODE_BACK;
    rasterizer.FrontCounterClockwise = FALSE;
    rasterizer.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizer.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizer.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizer.DepthClipEnable = TRUE;
    rasterizer.MultisampleEnable = FALSE;
    rasterizer.AntialiasedLineEnable = FALSE;
    rasterizer.ForcedSampleCount = 0;
    rasterizer.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    // ---------------------
    // --- Color Blender ---
    // ---------------------

    D3D12_BLEND_DESC blender = {};
    blender.AlphaToCoverageEnable = FALSE;
    blender.IndependentBlendEnable = FALSE;
    const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
    {
        FALSE,FALSE,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    };
    for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
        blender.RenderTarget[i] = defaultRenderTargetBlendDesc;

    // ---------------------
    // --- Depth Stencil ---
    // ---------------------

    D3D12_DEPTH_STENCIL_DESC depthStencil = {};
    depthStencil.DepthEnable = TRUE;
    depthStencil.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencil.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depthStencil.StencilEnable = FALSE;
    depthStencil.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    depthStencil.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
    { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
    depthStencil.FrontFace = defaultStencilOp;
    depthStencil.BackFace = defaultStencilOp;
    
    // -----------------------------------
    // --- Pipeline State Object (PSO) ---
    // -----------------------------------

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso = {};
    pso.pRootSignature = rootSignature;
    pso.VS = { reinterpret_cast<BYTE*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
    pso.PS = { reinterpret_cast<BYTE*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
    pso.BlendState = blender;
    pso.SampleMask = UINT_MAX;
    pso.RasterizerState = rasterizer;
    pso.DepthStencilState = depthStencil;
    pso.InputLayout = { inputLayout, 2 };
    pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso.NumRenderTargets = 1;
    pso.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    pso.SampleDesc.Count = graphics->Antialiasing();
    pso.SampleDesc.Quality = graphics->Quality();
    graphics->Device()->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&pipelineState));

    vertexShader->Release();
    pixelShader->Release();
}


// ------------------------------------------------------------------------------
//                                  WinMain                                      
// ------------------------------------------------------------------------------

int APIENTRY WinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    try
    {
        // cria motor e configura a janela
        Engine* engine = new Engine();
        engine->window->Mode(WINDOWED);
        engine->window->Size(1024, 720);
        engine->window->Color(25, 25, 25);
        engine->window->Title("Multi");
        engine->window->Icon(IDI_ICON);
        engine->window->Cursor(IDC_CURSOR);
        engine->window->LostFocus(Engine::Pause);
        engine->window->InFocus(Engine::Resume);

        // cria e executa a aplicação
        engine->Start(new Multi());

        // finaliza execução
        delete engine;
    }
    catch (Error & e)
    {
        // exibe mensagem em caso de erro
        MessageBox(nullptr, e.ToString().data(), "Multi", MB_OK);
    }

    return 0;
}

// ----------------------------------------------------------------------------

