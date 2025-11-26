#pragma once

namespace core {

    // Forward declare as *struct* to match Window.hpp
    struct Window;

    // Minimal detector contract. Students can extend with return values or reports.
    class Detector {
    public:
        virtual ~Detector() = default;
        virtual void detect(const Window& /*w*/) = 0;
    };

} // namespace core
