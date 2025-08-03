#include "core/graphics/rendering/factories/headers/VAOFactory.h"
#include "core/graphics/rendering/factories/headers/BufferFactory.h"
#include <stdexcept>
#include <utility>
// #include <lock_guard> // Uncomment if using mutex

// --- Constructor ---

VAOFactory::VAOFactory(BufferFactory& bufferFactory) noexcept
    : m_bufferFactory(bufferFactory) {}

// --- Private Helpers ---

std::shared_ptr<VertexArray> VAOFactory::findVAOOrThrow(VAOId id) const {
    // std::lock_guard<std::mutex> lock(m_mutex); // Uncomment for thread safety

    if (!id) [[unlikely]] {
        throw std::runtime_error("Invalid VAOId (0) provided.");
    }

    if (const auto it = m_userVAOs.find(id); it != m_userVAOs.end()) [[likely]] {
        return it->second;
    }

    throw std::runtime_error("VAOId not found: " + std::to_string(id.value));
}

std::shared_ptr<Buffer> VAOFactory::validateBuffer(BufferId bufferId, BufferType expectedType) const {
    auto buffer = m_bufferFactory.getBuffer(bufferId);

    if (!buffer) [[unlikely]] {
        throw std::runtime_error("BufferFactory::getBuffer returned nullptr for BufferId: " +
                               std::to_string(bufferId.value));
    }

    if (buffer->getType() != expectedType) [[unlikely]] {
        const char* expectedTypeName = (expectedType == BufferType::Vertex) ?
                                      "VERTEX_BUFFER" : "INDEX_BUFFER";
        throw std::runtime_error("Buffer is not a " + std::string(expectedTypeName) +
                               ". BufferId: " + std::to_string(bufferId.value));
    }

    return buffer;
}

// --- VAO Management API ---

VAOId VAOFactory::createVAO() {
    // std::lock_guard<std::mutex> lock(m_mutex); // Uncomment for thread safety

    VAOId newId{m_nextVAOId++};
    auto vao = std::make_shared<VertexArray>();
    vao->init();

    m_userVAOs.emplace(newId, std::move(vao));
    return newId;
}

void VAOFactory::bindVAO(VAOId id) const {
    findVAOOrThrow(id)->bind();
}

void VAOFactory::unbindVAO() noexcept {
    GL_CHECK(glBindVertexArray(0));
}

void VAOFactory::configureVertexAttributes(VAOId vaoId, BufferId bufferId,
                                          const std::vector<VertexAttribute>& attributes) const {
    auto vao = findVAOOrThrow(vaoId);
    auto buffer = validateBuffer(bufferId, BufferType::Vertex);

    vao->addVertexBuffer(buffer, attributes);
}

void VAOFactory::setIndexBuffer(VAOId vaoId, BufferId indexBufferId) const {
    auto vao = findVAOOrThrow(vaoId);
    auto buffer = validateBuffer(indexBufferId, BufferType::Index);

    vao->setIndexBuffer(buffer);
}

void VAOFactory::deleteVAO(VAOId id) {

    if (m_userVAOs.erase(id) == 0) [[unlikely]] {
        throw std::runtime_error("Attempted to delete non-existent VAOId: " +
                               std::to_string(id.value));
    }
}

std::shared_ptr<VertexArray> VAOFactory::getVAO(VAOId id) const noexcept {

    if (const auto it = m_userVAOs.find(id); it != m_userVAOs.end()) {
        return it->second;
    }
    return nullptr;
}

size_t VAOFactory::getVAOCount() const noexcept {
    return m_userVAOs.size();
}

bool VAOFactory::hasVAO(VAOId id) const noexcept {
    return m_userVAOs.find(id) != m_userVAOs.end();
}