# AdvancedGraphics Agent Handbook
*A compact reference for building and maintaining the high-level rendering API wrappers.*

---

## 2. Verified Building-Blocks
| # | Class / Enum / Struct | File | Purpose | Key Public API (signatures only) |
|---|-----------------------|------|---------|----------------------------------|
| 1 | **Buffer** | `rendering/core/headers/Buffer.h` | RAII VBO/EBO/UBO wrapper | `Buffer(BufferType, BufferUsage)` • `setData` • `updateData` • `reserve` • `bind` • `unbind` • `clear` • getters |
| 2 | **BufferFactory** | `rendering/factories/headers/BufferFactory.h` | Creates, pools & manages Buffers | `createBuffer` (overloads) • `createVertexBuffer` • `createIndexBuffer` • `updateBuffer` • `bindBuffer` • `unbindBuffer` • `deleteBuffer(s)` • `clear` • `optimize` • `getStats` |
| 3 | **VertexArray** | `rendering/core/headers/VertexArray.h` | RAII VAO wrapper | `init` • `bind / unbind` • `addVertexBuffer` • `setIndexBuffer` • stats getters |
| 4 | **VAOFactory** | `rendering/factories/headers/VAOFactory.h` | High-level VAO manager | `createVAO` • `bindVAO` • `unbindVAO` • `configureVertexAttributes` • `setIndexBuffer` • `deleteVAO` • misc getters |
| 5 | **BatchRenderer** | `rendering/batching/headers/BatchRenderer.h` | Efficient CPU-side batching | `begin / end / flush` • `addVertexData` • `addIndexData` • `addQuad` • `addTriangle` • stats getters |
| 6 | **BatchRendererFactory** | `rendering/factories/headers/BatchRendererFactory.h` | Static helpers to build BatchRenderer | `create` • `createDefault` • `createForSprites` |
| 7 | **DrawCommands** (static class) | `rendering/utils/headers/DrawCommands.h` | Thin, validated draw wrappers | `drawArrays*` • `drawElements*` • instanced / base-vertex / range / multi-draw • convenience (`drawFullscreenQuad`, etc.) • helpers (`validateDrawParams`, `getIndexSize`, …) |
| 8 | **graphics::Texture** | `textures/headers/Texture.h` | Image loading & GL texture object | `buildGLTexture` • `setCustomBuildFunction` • rich getters • `isValid` |
| 9 | **graphics::TextureManager** | `textures/headers/TextureManager.h` | Singleton cache & binder for Textures | `getInstance` • `createTexture` • `getTexture` • binding (`bindTexture`, `unbind*`, `unbindAllTextures`) • queries • `removeTexture` • `clearAllTextures` • logging hook |
|10 | **Shader** | `shaders/headers/Shader.h` | Modern program object & uniform handling | ctor `(vertexSrc?, fragSrc?)` • move / `~Shader` • `bind / unbind / use` • `operator[]` uniform access • `createUniformHandle` • `precacheUniforms` • `recompile` • debug prints |
|11 | **Uniform**, **UniformHandle** | same header | Strong, variant-based uniform accessors | assignment overloads; `apply`, `isSmart`, `setSmart` |
|12 | **GLUtils** utilities | `rendering/utils/headers/GLUtils.h` | Error checks, strong IDs, enums, caps | `checkGlError` • strong types `BufferId`, `VAOId`, `TextureId`, … • enums `BufferUsage`, `BufferType`, `TextureType`, `ShaderType` • `VertexAttribute` struct + helpers • `GLCapabilities` queries • debug-group helpers |

---

## 3. API Categories & Suggested Wrappers
| Category | Offer via `AdvancedGraphics` | Internals used |
|----------|-----------------------------|----------------|
| **Initialization** | `init()` → create factories, pre-query caps; `makeDefaultBatch()`; `createShaderFromFiles(vs,fs)` … | BufferFactory, VAOFactory, BatchRendererFactory, Shader |
| **Buffers & VAOs** | `createVertexBuffer`, `createIndexBuffer`, `createVAO`, `setVAOAttributes` | BufferFactory, VAOFactory, VertexAttribute |
| **Drawing** | `drawArrays`, `drawElements`, plus sugar `drawQuad`, `drawSpriteBatch` | DrawCommands, BatchRenderer |
| **Shaders** | `useShader`, `setUniform(handle,val)`, `recompileShader` | Shader, UniformHandle |
| **Textures** | `loadTexture`, `bindTexture(slot)`, `unbindTexture`, `clearTextures` | TextureManager, Texture |
| **Cleanup** | `shutdown()` → clear pools, delete buffers / VAOs, unbind textures | BufferFactory::clear, VAOFactory::deleteVAO, TextureManager::clearAllTextures |

## Typical Usage Flow
```cpp
using namespace advanced_gfx;

void demo()
{
    init();                                    // Initialise factories / GL caps

    auto shader = createShaderFromFiles("basic.vert", "basic.frag");
    auto tex    = loadTexture("logo.png");

    auto vbo = createVertexBuffer(vertices, sizeof(vertices));
    auto ibo = createIndexBuffer(indices, sizeof(indices));

    auto vao = createVAO();
    setVAOAttributes(vao, vbo, {
        VertexAttribute::position(0, sizeof(VertexData), 0),
        VertexAttribute::texCoord(1, sizeof(VertexData), offsetof(VertexData, texCoord))
    });
    setIndexBuffer(vao, ibo);

    shader.bind();
    tex->bind(0);
    vao.bind();

    drawElements(DrawCommands::PrimitiveType::Triangles, indexCount);   // forwards to DrawCommands
}
```
## Appendix: Data Structures & Their Locations

| Data Structure | Kind | Header File |
|----------------|------|-------------|
| `StrongId<Tag, ValueType>` | template struct | `rendering/utils/headers/GLUtils.h` |
| `BufferIdTag`, `VAOIdTag`, `TextureIdTag`, `ShaderIdTag`, `ProgramIdTag`, `FramebufferIdTag` | tag structs | `rendering/utils/headers/GLUtils.h` |
| `BufferId`, `VAOId`, `TextureId`, `ShaderId`, `ProgramId`, `FramebufferId` | type aliases (using) | `rendering/utils/headers/GLUtils.h` |
| `BufferUsage` | `enum class` | `rendering/utils/headers/GLUtils.h` |
| `BufferType` | `enum class` | `rendering/utils/headers/GLUtils.h` |
| `TextureType` | `enum class` | `rendering/utils/headers/GLUtils.h` |
| `ShaderType` | `enum class` | `rendering/utils/headers/GLUtils.h` |
| `VertexAttribute` | struct | `rendering/utils/headers/GLUtils.h` |
| `GLCapabilities` | utility class | `rendering/utils/headers/GLUtils.h` |
| `DrawCommands::PrimitiveType` | `enum class` | `rendering/utils/headers/DrawCommands.h` |
| `DrawCommands::IndexType` | `enum class` | `rendering/utils/headers/DrawCommands.h` |
| `VertexData` | struct | `rendering/batching/headers/BatchRenderer.h` |
| `BufferFactory::Config` | struct | `rendering/factories/headers/BufferFactory.h` |
| `BufferFactory::BufferInfo` | struct | `rendering/factories/headers/BufferFactory.h` |
| `BufferFactory::Stats` | struct | `rendering/factories/headers/BufferFactory.h` |
| `TextureCache` | struct | `textures/headers/TextureManager.h` |
| `UniformValue` | `using` alias (`std::variant`) | `shaders/headers/Shader.h` |
| `Uniform` | struct / class | `shaders/headers/Shader.h` |
| `UniformHandle` | lightweight class | `shaders/headers/Shader.h` |
| `Shader::CommonUniforms` | internal struct | `shaders/headers/Shader.h` |
