#pragma once

#include "core/graphics/rendering/core/headers/VertexArray.h"
#include "core/graphics/rendering/factories/headers/BufferFactory.h"
#include "core/graphics/rendering/utils/headers/GLUtils.h"
#include <memory>
#include <unordered_map>
#include <vector>

// Forward declarations to reduce compilation dependencies
struct VertexAttribute;

/**
 * @brief Manages the creation, configuration, binding, and deletion of Vertex Array Objects (VAOs).
 *
 * Provides a high-level interface for VAO management, associating user-defined
 * IDs (VAOId) with actual VertexArray objects. Interacts with a BufferFactory
 * to link Buffers (identified by BufferId) to VAOs.
 */
class VAOFactory {
private:
    std::unordered_map<VAOId, std::shared_ptr<VertexArray>> m_userVAOs;
    BufferFactory& m_bufferFactory;
    uint32_t m_nextVAOId = 1;
    // mutable std::mutex m_mutex; // Uncomment for thread safety

    /**
     * @brief Finds a VAO by its ID, throwing if not found.
     * @param id The VAOId to find.
     * @return A shared_ptr to the found VertexArray.
     * @throws std::runtime_error If the VAOId is invalid or not found.
     */
    [[nodiscard]] std::shared_ptr<VertexArray> findVAOOrThrow(VAOId id) const;

    /**
     * @brief Validates that a buffer exists and has the expected type.
     * @param bufferId The buffer ID to validate.
     * @param expectedType The expected buffer type.
     * @return The validated buffer.
     * @throws std::runtime_error If validation fails.
     */
    [[nodiscard]] std::shared_ptr<Buffer> validateBuffer(BufferId bufferId, BufferType expectedType) const;

public:
    /**
     * @brief Constructs a VAOFactory.
     * @param bufferFactory A reference to the BufferFactory, used to look up buffers.
     */
    explicit VAOFactory(BufferFactory& bufferFactory) noexcept;

    ~VAOFactory() = default;

    // Non-copyable, non-movable
    VAOFactory(const VAOFactory&) = delete;
    VAOFactory& operator=(const VAOFactory&) = delete;
    VAOFactory(VAOFactory&&) = delete;
    VAOFactory& operator=(VAOFactory&&) = delete;

    // --- VAO Management API ---

    /**
     * @brief Creates a new VertexArray object and returns a unique ID.
     * @return A unique VAOId handle for the created VAO.
     */
    VAOId createVAO();

    /**
     * @brief Binds the specified VAO, making it the active vertex array state.
     * @param id The ID of the VAO to bind.
     * @throws std::runtime_error If the VAOId is invalid or not found.
     */
    void bindVAO(VAOId id) const;

    /**
     * @brief Unbinds the currently bound VAO (binds VAO 0).
     */
    static void unbindVAO() noexcept;

    /**
     * @brief Configures vertex attributes for a VAO using a specific Buffer.
     * @param vaoId The ID of the VAO to configure.
     * @param bufferId The ID of the Buffer (must be a VERTEX_BUFFER).
     * @param attributes A vector describing the vertex attributes layout.
     * @throws std::runtime_error If validation fails.
     */
    void configureVertexAttributes(VAOId vaoId, BufferId bufferId,
                                 const std::vector<VertexAttribute>& attributes) const;

    /**
     * @brief Associates an Index Buffer with a VAO.
     * @param vaoId The ID of the VAO.
     * @param indexBufferId The ID of the Buffer (must be an INDEX_BUFFER).
     * @throws std::runtime_error If validation fails.
     */
    void setIndexBuffer(VAOId vaoId, BufferId indexBufferId) const;

    /**
     * @brief Deletes a VAO, freeing its GPU resources.
     * @param id The ID of the VAO to delete.
     * @throws std::runtime_error If the ID doesn't exist.
     */
    void deleteVAO(VAOId id);

    /**
     * @brief Retrieves the underlying VertexArray object.
     * @param id The VAOId.
     * @return A shared_ptr to the VertexArray, or nullptr if not found.
     */
    [[nodiscard]] std::shared_ptr<VertexArray> getVAO(VAOId id) const noexcept;

    /**
     * @brief Returns the number of managed VAOs.
     * @return The count of active VAOs.
     */
    [[nodiscard]] size_t getVAOCount() const noexcept;

    /**
     * @brief Checks if a VAO ID exists.
     * @param id The VAOId to check.
     * @return true if the VAO exists, false otherwise.
     */
    [[nodiscard]] bool hasVAO(VAOId id) const noexcept;
};