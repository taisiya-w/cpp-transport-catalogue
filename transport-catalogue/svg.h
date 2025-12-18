#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <variant>

namespace svg {

struct Rgb {
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    
    Rgb() = default;
    Rgb(uint8_t red, uint8_t green, uint8_t blue) : red(red), green(green), blue(blue) {}
};

struct Rgba {
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    double opacity = 1.0;
    
    Rgba() = default;
    Rgba(uint8_t red, uint8_t green, uint8_t blue, double opacity) 
        : red(red), green(green), blue(blue), opacity(opacity) {}
};

using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;
inline const Color NoneColor{"none"};

inline std::ostream& operator<<(std::ostream& out, const Color& color) {
    std::visit([&out](const auto& value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            out << "none";
        } else if constexpr (std::is_same_v<T, std::string>) {
            out << value;
        } else if constexpr (std::is_same_v<T, Rgb>) {
            out << "rgb(" << static_cast<int>(value.red) << "," 
                << static_cast<int>(value.green) << "," 
                << static_cast<int>(value.blue) << ")";
        } else if constexpr (std::is_same_v<T, Rgba>) {
            out << "rgba(" << static_cast<int>(value.red) << "," 
                << static_cast<int>(value.green) << "," 
                << static_cast<int>(value.blue) << "," 
                << value.opacity << ")";
        }
    }, color);
    return out;
}

enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
};

enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};

inline std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cap) {
    switch (line_cap) {
        case StrokeLineCap::BUTT: out << "butt"; break;
        case StrokeLineCap::ROUND: out << "round"; break;
        case StrokeLineCap::SQUARE: out << "square"; break;
    }
    return out;
}

inline std::ostream& operator<<(std::ostream& out, StrokeLineJoin line_join) {
    switch (line_join) {
        case StrokeLineJoin::ARCS: out << "arcs"; break;
        case StrokeLineJoin::BEVEL: out << "bevel"; break;
        case StrokeLineJoin::MITER: out << "miter"; break;
        case StrokeLineJoin::MITER_CLIP: out << "miter-clip"; break;
        case StrokeLineJoin::ROUND: out << "round"; break;
    }
    return out;
}

struct Point {
    double x = 0;
    double y = 0;

    Point() = default;
    Point(double x, double y) : x(x), y(y) {}
};

struct RenderContext {
    std::ostream& out;
    int indent_step = 0;
    int indent = 0;

    RenderContext(std::ostream& out) : out(out) {}
    
    RenderContext(std::ostream& out, int indent_step, int indent = 0)
        : out(out), indent_step(indent_step), indent(indent) {}

    RenderContext Indented() const {
        return {out, indent_step, indent + indent_step};
    }

    void RenderIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }
};

class Object {
public:
    void Render(const RenderContext& context) const;
    virtual ~Object() = default;

private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};

template <typename Derived>
class PathProps {
public:
    Derived& SetFillColor(Color color) {
        fill_color_ = std::move(color);
        return static_cast<Derived&>(*this);
    }
    
    Derived& SetStrokeColor(Color color) {
        stroke_color_ = std::move(color);
        return static_cast<Derived&>(*this);
    }
    
    Derived& SetStrokeWidth(double width) {
        stroke_width_ = width;
        return static_cast<Derived&>(*this);
    }
    
    Derived& SetStrokeLineCap(StrokeLineCap line_cap) {
        stroke_linecap_ = line_cap;
        return static_cast<Derived&>(*this);
    }
    
    Derived& SetStrokeLineJoin(StrokeLineJoin line_join) {
        stroke_linejoin_ = line_join;
        return static_cast<Derived&>(*this);
    }

protected:
    void RenderAttrs(std::ostream& out) const {
        if (!std::holds_alternative<std::monostate>(fill_color_)) {
            out << " fill=\"" << fill_color_ << "\"";
        }
        if (!std::holds_alternative<std::monostate>(stroke_color_)) {
            out << " stroke=\"" << stroke_color_ << "\"";
        }
        if (stroke_width_) {
            out << " stroke-width=\"" << *stroke_width_ << "\"";
        }
        if (stroke_linecap_) {
            out << " stroke-linecap=\"" << *stroke_linecap_ << "\"";
        }
        if (stroke_linejoin_) {
            out << " stroke-linejoin=\"" << *stroke_linejoin_ << "\"";
        }
    }

private:
    Color fill_color_;
    Color stroke_color_;
    std::optional<double> stroke_width_;
    std::optional<StrokeLineCap> stroke_linecap_;
    std::optional<StrokeLineJoin> stroke_linejoin_;
};

class Circle final : public Object, public PathProps<Circle> {
public:
    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);

private:
    void RenderObject(const RenderContext& context) const override;
    Point center_;
    double radius_ = 1.0;
};

class Polyline final : public Object, public PathProps<Polyline> {
public:
    Polyline& AddPoint(Point point);

private:
    void RenderObject(const RenderContext& context) const override;
    std::vector<Point> points_;
};

class Text final : public Object, public PathProps<Text> {
public:
    Text& SetPosition(Point pos);
    Text& SetOffset(Point offset);
    Text& SetFontSize(uint32_t size);
    Text& SetFontFamily(std::string font_family);
    Text& SetFontWeight(std::string font_weight);
    Text& SetData(std::string data);

private:
    void RenderObject(const RenderContext& context) const override;
    std::string EscapeString(const std::string& data) const;

    Point position_{0.0, 0.0};
    Point offset_{0.0, 0.0};
    uint32_t font_size_ = 1;
    std::string font_family_;
    std::string font_weight_;
    std::string data_;
};

class ObjectContainer {
public:
    template <typename Obj>
    void Add(Obj obj) {
        AddPtr(std::make_unique<Obj>(std::move(obj)));
    }

    virtual ~ObjectContainer() = default;

protected:
    virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;
};

class Drawable {
public:
    virtual void Draw(ObjectContainer& container) const = 0;
    virtual ~Drawable() = default;
};

class Document : public ObjectContainer {
public:
    void Render(std::ostream& out) const;
    std::string RenderToString() const;
    void AddPtr(std::unique_ptr<Object>&& obj) override;
private:
    std::vector<std::unique_ptr<Object>> objects_;
};

}  // namespace svg