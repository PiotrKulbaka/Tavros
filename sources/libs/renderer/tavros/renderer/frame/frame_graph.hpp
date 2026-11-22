#pragma once

#include <tavros/core/containers/vector.hpp>

#include <tavros/renderer/frame/render_pass.hpp>
#include <tavros/renderer/frame/render_pipeline.hpp>

namespace tavros::renderer
{

    class render_system;

    /**
     * @brief Builds a `render_pipeline` from a set of named, dependency-ordered render passes.
     *
     * Passes are registered via `register_pass<Stage>(name)` and may declare dependencies
     * on other passes by name through the returned `graph_builder`. `compile_for` then
     * performs a depth-first topological sort over the requested passes, instantiating
     * each pass exactly once and appending it to the resulting pipeline in dependency order.
     */
    class frame_graph
    {
    private:
        using pass_factory = core::unique_ptr<render_pass> (*)(render_system*);
        using pipeline_t = core::vector<core::unique_ptr<render_pass>>;

        struct node
        {
            core::string               name;
            core::vector<core::string> dependencies;
            pass_factory               create = nullptr;
        };

    public:
        /**
         * @brief Fluent helper returned by `register_pass` for declaring pass dependencies.
         */
        class graph_builder
        {
        public:
            /**
             * @brief Declares that the pass this builder was created for depends on `passes`.
             * @param passes Names of previously registered passes.
             * @return *this, to allow chaining.
             */
            graph_builder& depends_on(core::buffer_view<const core::string_view> passes);

        private:
            friend class frame_graph;

            graph_builder(frame_graph* graph, core::string_view name) noexcept
                : m_graph(graph)
                , m_name(name)
            {
                TAV_ASSERT(graph);
            }

        private:
            frame_graph*      m_graph = nullptr;
            core::string_view m_name;
        };

    public:
        explicit frame_graph(render_system* rs) noexcept;

        /**
         * @brief Registers a render pass under `name`.
         *
         * `Stage` must derive from `render_pass` and be constructible from a `render_system*`.
         * The pass is not instantiated until `compile_for` resolves it into a pipeline.
         *
         * @param name Unique name identifying the pass within this graph.
         * @return A `graph_builder` for declaring this pass's dependencies.
         */
        template<class Stage>
        graph_builder register_pass(core::string_view name)
        {
            static_assert(std::is_base_of_v<render_pass, Stage>, "Stage must be derived from render_pass");

            TAV_ASSERT(!name.empty());
            TAV_ASSERT(find_node(name) == nullptr);

            node new_node;
            new_node.name = name;

            new_node.create = [](render_system* rs) -> core::unique_ptr<render_pass> {
                return core::make_unique<Stage>(rs);
            };

            m_nodes.push_back(std::move(new_node));

            return graph_builder(this, m_nodes.back().name);
        }

        /**
         * @brief Compiles the requested passes (and their transitive dependencies) into a pipeline.
         *
         * @param passes Names of the passes that must be present in the resulting pipeline.
         *               Dependencies of these passes are pulled in automatically.
         * @return A `render_pipeline` with passes ordered so that every dependency
         *         precedes the pass that depends on it.
         *
         * @note Asserts on unknown pass names or on a dependency cycle.
         */
        core::unique_ptr<render_pipeline> compile_for(core::buffer_view<const core::string_view> passes);

    private:
        void add_dependency(core::string_view pass, core::string_view dependency);

        void compile_node(
            size_t                           index,
            core::vector<bool>&              visited,
            core::vector<bool>&              visiting,
            pipeline_t&                      pipeline,
            core::vector<core::string_view>& order
        );

        node*       find_node(core::string_view name) noexcept;
        const node* find_node(core::string_view name) const noexcept;
        size_t      find_node_index(core::string_view name) const noexcept;

    private:
        static constexpr size_t invalid_index = static_cast<size_t>(-1);

    private:
        render_system*     m_render_system = nullptr;
        core::vector<node> m_nodes;
    };

} // namespace tavros::renderer
