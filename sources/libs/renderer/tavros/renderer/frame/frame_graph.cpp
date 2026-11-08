#include <tavros/renderer/frame/frame_graph.hpp>

#include <tavros/core/logger/logger.hpp>

namespace
{
    tavros::core::logger logger("frame_graph");
}

namespace tavros::renderer
{

    frame_graph::graph_builder& frame_graph::graph_builder::depends_on(core::buffer_view<const core::string_view> passes)
    {
        TAV_ASSERT(m_graph);

        for (const core::string_view pass : passes) {
            m_graph->add_dependency(m_name, pass);
        }

        return *this;
    }

    frame_graph::frame_graph(render_system* rs) noexcept
        : m_render_system(rs)
    {
        TAV_ASSERT(rs);
    }

    core::unique_ptr<render_pipeline> frame_graph::compile_for(core::buffer_view<const core::string_view> passes)
    {
        if (passes.empty()) {
            logger.error("compile_for() called with an empty pass list, resulting pipeline will be empty");
            return nullptr;
        }

        pipeline_t pipeline;
        pipeline.reserve(passes.size());

        core::vector<core::string_view> order;
        core::vector<bool>              visited(m_nodes.size(), false);
        core::vector<bool>              visiting(m_nodes.size(), false);

        for (const core::string_view pass : passes) {
            const size_t index = find_node_index(pass);

            if (index == invalid_index) {
                logger.error("cannot compile frame graph: pass '{}' is not registered", pass);
                return nullptr;
            }

            TAV_ASSERT(index != invalid_index);

            compile_node(index, visited, visiting, pipeline, order);
        }

        logger.info("frame graph compiled, {} pass(es) accepted:", order.size());
        for (size_t i = 0; i < order.size(); ++i) {
            logger.info("  {}. {}", i + 1, order[i]);
        }

        auto rp = render_pipeline(std::move(pipeline));

        return core::make_unique<render_pipeline>(std::move(rp));
    }

    void frame_graph::add_dependency(core::string_view pass, core::string_view dependency)
    {
        node* n = find_node(pass);

        if (!n) {
            logger.error("cannot add dependency '{}' to unknown pass '{}'", dependency, pass);
        }

        TAV_ASSERT(n);

        if (!find_node(dependency)) {
            logger.error("pass '{}' depends on unknown pass '{}'", pass, dependency);
        }

        TAV_ASSERT(find_node(dependency));

        logger.debug("pass '{}' now depends on '{}'", pass, dependency);

        n->dependencies.emplace_back(dependency);
    }

    void frame_graph::compile_node(size_t index, core::vector<bool>& visited, core::vector<bool>& visiting, pipeline_t& pipeline, core::vector<core::string_view>& order)
    {
        TAV_ASSERT(index < m_nodes.size());

        if (visited[index]) {
            return;
        }

        node& n = m_nodes[index];

        // A node is already being visited, therefore we have
        // encountered a cycle in the graph.
        if (visiting[index]) {
            logger.error("frame graph compilation failed: dependency cycle detected at pass '{}'", n.name);
        }

        TAV_ASSERT(!visiting[index]);

        visiting[index] = true;

        logger.debug("resolving dependencies for pass '{}'", n.name);

        // Resolve all dependencies first.
        for (const core::string& dependency : n.dependencies) {
            const size_t dependency_index = find_node_index(dependency);

            if (dependency_index == invalid_index) {
                logger.error("pass '{}' depends on unknown pass '{}'", n.name, dependency);
            }

            TAV_ASSERT(dependency_index != invalid_index);

            compile_node(dependency_index, visited, visiting, pipeline, order);
        }

        visiting[index] = false;
        visited[index] = true;

        TAV_ASSERT(n.create);

        logger.debug("instantiating pass '{}'", n.name);

        pipeline.push_back(n.create(m_render_system));
        order.push_back(n.name);
    }

    frame_graph::node* frame_graph::find_node(core::string_view name) noexcept
    {
        for (node& n : m_nodes) {
            if (n.name == name) {
                return &n;
            }
        }

        return nullptr;
    }

    const frame_graph::node* frame_graph::find_node(core::string_view name) const noexcept
    {
        for (const node& n : m_nodes) {
            if (n.name == name) {
                return &n;
            }
        }

        return nullptr;
    }

    size_t frame_graph::find_node_index(core::string_view name) const noexcept
    {
        for (size_t i = 0; i < m_nodes.size(); ++i) {
            if (m_nodes[i].name == name) {
                return i;
            }
        }

        return invalid_index;
    }

} // namespace tavros::renderer
