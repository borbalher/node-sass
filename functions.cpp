#ifndef SASS_PRELEXER_INCLUDED
#include "prelexer.hpp"
#endif
#include "functions.hpp"
#include "error.hpp"
#include <iostream>
#include <cmath>
using std::cerr; using std::endl;

namespace Sass {
  namespace Functions {
    
    static void eval_error(string message, size_t line_number, const char* file_name)
    {
      string fn;
      if (file_name) {
        const char* end = Prelexer::string_constant(file_name);
        if (end) fn = string(file_name, end - file_name);
        else fn = string(file_name);
      }
      throw Error(Error::evaluation, line_number, fn, message);
    }

    // RGB Functions ///////////////////////////////////////////////////////

    Function_Descriptor rgb_descriptor = 
    { "rgb", "$red", "$green", "$blue", 0 };
    Node rgb(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node r(bindings[parameters[0]]);
      Node g(bindings[parameters[1]]);
      Node b(bindings[parameters[2]]);
      if (!(r.type == Node::number && g.type == Node::number && b.type == Node::number)) {
        eval_error("arguments for rgb must be numbers", r.line_number, r.file_name);
      }
      Node color(Node::numeric_color, 0, 4);
      color << r << g << b << Node(0, 1.0);
      return color;
    }

    Function_Descriptor rgba_4_descriptor = 
    { "rgba", "$red", "$green", "$blue", "$alpha", 0 };
    Node rgba_4(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node r(bindings[parameters[0]]);
      Node g(bindings[parameters[1]]);
      Node b(bindings[parameters[2]]);
      Node a(bindings[parameters[3]]);
      if (!(r.type == Node::number && g.type == Node::number && b.type == Node::number && a.type == Node::number)) {
        eval_error("arguments for rgba must be numbers", r.line_number, r.file_name);
      }
      Node color(Node::numeric_color, 0, 4);
      color << r << g << b << a;
      return color;
    }
    
    Function_Descriptor rgba_2_descriptor = 
    { "rgba", "$color", "$alpha", 0 };
    Node rgba_2(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node color(bindings[parameters[0]].clone());
      color[3] = bindings[parameters[1]];
      return color;
    }
    
    Function_Descriptor red_descriptor =
    { "red", "$color", 0 };
    Node red(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node color(bindings[parameters[0]]);
      if (color.type != Node::numeric_color) eval_error("argument to red must be a color", color.line_number, color.file_name);
      return color[0];
    }
    
    Function_Descriptor green_descriptor =
    { "green", "$color", 0 };
    Node green(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node color(bindings[parameters[0]]);
      if (color.type != Node::numeric_color) eval_error("argument to green must be a color", color.line_number, color.file_name);
      return color[1];
    }
    
    Function_Descriptor blue_descriptor =
    { "blue", "$color", 0 };
    Node blue(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node color(bindings[parameters[0]]);
      if (color.type != Node::numeric_color) eval_error("argument to blue must be a color", color.line_number, color.file_name);
      return color[2];
    }
    
    Node mix_impl(Node color1, Node color2, double weight = 50) {
      if (!(color1.type == Node::numeric_color && color2.type == Node::numeric_color)) {
        eval_error("first two arguments to mix must be colors", color1.line_number, color1.file_name);
      }
      double p = weight/100;
      double w = 2*p - 1;
      double a = color1[3].content.numeric_value - color2[3].content.numeric_value;
      
      double w1 = (((w * a == -1) ? w : (w + a)/(1 + w*a)) + 1)/2.0;
      double w2 = 1 - w1;
      
      Node mixed(Node::numeric_color, color1.line_number, 4);
      for (int i = 0; i < 3; ++i) {
        mixed << Node(mixed.line_number, w1*color1[i].content.numeric_value +
                                         w2*color2[i].content.numeric_value);
      }
      double alpha = color1[3].content.numeric_value*p + color2[3].content.numeric_value*(1-p);
      mixed << Node(mixed.line_number, alpha);
      return mixed;
    }
    
    Function_Descriptor mix_2_descriptor =
    { "mix", "$color1", "$color2", 0 };
    Node mix_2(const vector<Token>& parameters, map<Token, Node>& bindings) {
      return mix_impl(bindings[parameters[0]], bindings[parameters[1]]);
    }
    
    Function_Descriptor mix_3_descriptor =
    { "mix", "$color1", "$color2", "$weight", 0 };
    Node mix_3(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node percentage(bindings[parameters[2]]);
      if (!(percentage.type == Node::number || percentage.type == Node::numeric_percentage || percentage.type == Node::numeric_dimension)) {
        eval_error("third argument to mix must be numeric", percentage.line_number, percentage.file_name);
      }
      return mix_impl(bindings[parameters[0]],
                      bindings[parameters[1]],
                      percentage.numeric_value());
    }
    
    // HSL Functions ///////////////////////////////////////////////////////
    
    double h_to_rgb(double m1, double m2, double h) {
      if (h < 0) ++h;
      if (h > 1) --h;
      if (h*6.0 < 1) return m1 + (m2 - m1)*h*6;
      if (h*2.0 < 1) return m2;
      if (h*3.0 < 2) return m1 + (m2 - m1) * (2.0/3.0 - h)*6;
      return m1;
    }

    Node hsla_impl(double h, double s, double l, double a = 1) {
      h = static_cast<double>(((static_cast<int>(h) % 360) + 360) % 360) / 360.0;
      s = s / 100.0;
      l = l / 100.0;

      double m2;
      if (l <= 0.5) m2 = l*(s+1.0);
      else m2 = l+s-l*s;
      double m1 = l*2-m2;
      double r = h_to_rgb(m1, m2, h+1.0/3.0) * 255.0;
      double g = h_to_rgb(m1, m2, h) * 255.0;
      double b = h_to_rgb(m1, m2, h-1.0/3.0) * 255.0;
      
      return Node(0, r, g, b, a);
    }

    Function_Descriptor hsla_descriptor =
    { "hsla", "$hue", "$saturation", "$lightness", "$alpha", 0 };
    Node hsla(const vector<Token>& parameters, map<Token, Node>& bindings) {
      if (!(bindings[parameters[0]].is_numeric() &&
            bindings[parameters[1]].is_numeric() &&
            bindings[parameters[2]].is_numeric() &&
            bindings[parameters[3]].is_numeric())) {
        eval_error("arguments to hsla must be numeric", bindings[parameters[0]].line_number, bindings[parameters[0]].file_name);
      }  
      double h = bindings[parameters[0]].numeric_value();
      double s = bindings[parameters[1]].numeric_value();
      double l = bindings[parameters[2]].numeric_value();
      double a = bindings[parameters[3]].numeric_value();
      Node color(hsla_impl(h, s, l, a));
      color.line_number = bindings[parameters[0]].line_number;
      return color;
    }
    
    Function_Descriptor hsl_descriptor =
    { "hsl", "$hue", "$saturation", "$lightness", 0 };
    Node hsl(const vector<Token>& parameters, map<Token, Node>& bindings) {
      if (!(bindings[parameters[0]].is_numeric() &&
            bindings[parameters[1]].is_numeric() &&
            bindings[parameters[2]].is_numeric())) {
        eval_error("arguments to hsl must be numeric", bindings[parameters[0]].line_number, bindings[parameters[0]].file_name);
      }  
      double h = bindings[parameters[0]].numeric_value();
      double s = bindings[parameters[1]].numeric_value();
      double l = bindings[parameters[2]].numeric_value();
      Node color(hsla_impl(h, s, l));
      color.line_number = bindings[parameters[0]].line_number;
      return color;
    }
    
    Function_Descriptor invert_descriptor =
    { "invert", "$color", 0 };
    Node invert(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node orig(bindings[parameters[0]]);
      if (orig.type != Node::numeric_color) eval_error("argument to invert must be a color", orig.line_number, orig.file_name);
      return Node(orig.line_number,
                  255 - orig[0].content.numeric_value,
                  255 - orig[1].content.numeric_value,
                  255 - orig[2].content.numeric_value,
                  orig[3].content.numeric_value);
    }
    
    // Opacity Functions ///////////////////////////////////////////////////
    
    Function_Descriptor alpha_descriptor =
    { "alpha", "$color", 0 };
    Function_Descriptor opacity_descriptor =
    { "opacity", "$color", 0 };
    Node alpha(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node color(bindings[parameters[0]]);
      if (color.type != Node::numeric_color) eval_error("argument to alpha must be a color", color.line_number, color.file_name);
      return color[3];
    }
    
    Function_Descriptor opacify_descriptor =
    { "opacify", "$color", "$amount", 0 };
    Function_Descriptor fade_in_descriptor =
    { "fade_in", "$color", "$amount", 0 };
    Node opacify(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node cpy(bindings[parameters[0]].clone());
      cpy[3].content.numeric_value += bindings[parameters[1]].content.numeric_value;
      if (cpy[3].content.numeric_value >= 1) cpy[3].content.numeric_value = 1;
      return cpy;
    }
    
    Function_Descriptor transparentize_descriptor =
    { "transparentize", "$color", "$amount", 0 };
    Function_Descriptor fade_out_descriptor =
    { "fade_out", "$color", "$amount", 0 };
    Node transparentize(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node cpy(bindings[parameters[0]].clone());
      cpy[3].content.numeric_value -= bindings[parameters[1]].content.numeric_value;
      if (cpy[3].content.numeric_value <= 0) cpy[3].content.numeric_value = 0;
      return cpy;
    }
      
    // String Functions ////////////////////////////////////////////////////
    
    Function_Descriptor unquote_descriptor =
    { "unquote", "$string", 0 };
    Node unquote(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node cpy(bindings[parameters[0]].clone());
      cpy.unquoted = true;
      return cpy;
    }
    
    Function_Descriptor quote_descriptor =
    { "quote", "$string", 0 };
    Node quote(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node cpy(bindings[parameters[0]].clone());
      // check the types -- will probably be an identifier
      cpy.type = Node::string_constant;
      cpy.unquoted = false;
      return cpy;
    }
    
    // Number Functions ////////////////////////////////////////////////////
    
    Function_Descriptor percentage_descriptor =
    { "percentage", "$value", 0 };
    Node percentage(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node cpy(bindings[parameters[0]].clone());
      // TO DO: make sure it's not already a percentage
      cpy.content.numeric_value = cpy.content.numeric_value * 100;
      cpy.type = Node::numeric_percentage;
      return cpy;
    }

    Function_Descriptor round_descriptor =
    { "round", "$value", 0 };
    Node round(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node cpy(bindings[parameters[0]].clone());
      if (cpy.type == Node::numeric_dimension) {
        cpy.content.dimension.numeric_value = std::floor(cpy.content.dimension.numeric_value + 0.5);
      }
      else {
        cpy.content.numeric_value = std::floor(cpy.content.numeric_value + 0.5);
      }
      return cpy;
    }

    Function_Descriptor ceil_descriptor =
    { "ceil", "$value", 0 };
    Node ceil(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node cpy(bindings[parameters[0]].clone());
      if (cpy.type == Node::numeric_dimension) {
        cpy.content.dimension.numeric_value = std::ceil(cpy.content.dimension.numeric_value);
      }
      else {
        cpy.content.numeric_value = std::ceil(cpy.content.numeric_value);
      }
      return cpy;
    }

    Function_Descriptor floor_descriptor =
    { "floor", "$value", 0 };
    Node floor(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node cpy(bindings[parameters[0]].clone());
      if (cpy.type == Node::numeric_dimension) {
        cpy.content.dimension.numeric_value = std::floor(cpy.content.dimension.numeric_value);
      }
      else {
        cpy.content.numeric_value = std::floor(cpy.content.numeric_value);
      }
      return cpy;
    }

    Function_Descriptor abs_descriptor =
    { "abs", "$value", 0 };
    Node abs(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node cpy(bindings[parameters[0]].clone());
      if (cpy.type == Node::numeric_dimension) {
        cpy.content.dimension.numeric_value = std::fabs(cpy.content.dimension.numeric_value);
      }
      else {
        cpy.content.numeric_value = std::fabs(cpy.content.numeric_value);
      }
      return cpy;
    }
    
    // List Functions //////////////////////////////////////////////////////

    Function_Descriptor length_descriptor =
    { "length", "$list", 0 };
    Node length(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node arg(bindings[parameters[0]]);
      if (arg.type == Node::space_list || arg.type == Node::comma_list) {
        return Node(arg.line_number, arg.size());
      }
      else if (arg.type == Node::nil) {
        return Node(arg.line_number, 0);
      }
      else {
        return Node(arg.line_number, 1);
      }
    }
    
    Function_Descriptor nth_descriptor =
    { "nth", "$list", "$n", 0 };
    Node nth(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node l(bindings[parameters[0]]);
      // TO DO: check for empty list
      if (l.type != Node::space_list && l.type != Node::comma_list) {
        l = Node(Node::space_list, l.line_number, 1) << l;
      }
      return l[bindings[parameters[1]].content.numeric_value - 1];
    }
    
    extern const char separator_kwd[] = "$separator";
    Node join_impl(const vector<Token>& parameters, map<Token, Node>& bindings, bool has_sep = false) {
      Node l1(bindings[parameters[0]]);
      if (l1.type != Node::space_list && l1.type != Node::comma_list && l1.type != Node::nil) {
        l1 = Node(Node::space_list, l1.line_number, 1) << l1;
        cerr << "listified singleton" << endl;
      }
      Node l2(bindings[parameters[1]]);
      if (l2.type != Node::space_list && l2.type != Node::comma_list && l2.type != Node::nil) {
        l2 = Node(Node::space_list, l2.line_number, 1) << l2;
        cerr << "listified singleton" << endl;
      }
      
      if (l1.type == Node::nil && l2.type == Node::nil) return Node(Node::nil, l1.line_number);

      size_t size = 0;
      if (l1.type != Node::nil) size += l1.size();
      if (l2.type != Node::nil) size += l2.size();
      
      Node lr(Node::space_list, l1.line_number, size);
      if (has_sep) {
        string sep(bindings[parameters[2]].content.token.unquote());
        if (sep == "comma") lr.type = Node::comma_list;
        // TO DO: check for "space" or "auto"
      }
      else if (l1.type != Node::nil) lr.type = l1.type;
      else if (l2.type != Node::nil) lr.type = l2.type;
      
      if (l1.type != Node::nil) lr += l1;
      if (l2.type != Node::nil) lr += l2;
      return lr;
    }
    
    Function_Descriptor join_2_descriptor =
    { "join", "$list1", "$list2", 0 };
    Node join_2(const vector<Token>& parameters, map<Token, Node>& bindings) {
      return join_impl(parameters, bindings);
    }
    
    Function_Descriptor join_3_descriptor =
    { "join", "$list1", "$list2", "$separator", 0 };
    Node join_3(const vector<Token>& parameters, map<Token, Node>& bindings) {
      return join_impl(parameters, bindings, true);
    }
    
    // Introspection Functions /////////////////////////////////////////////
    
    extern const char number_name[] = "number";
    extern const char string_name[] = "string";
    extern const char bool_name[]   = "bool";
    extern const char color_name[]  = "color";
    extern const char list_name[]   = "list";
    
    Function_Descriptor type_of_descriptor =
    { "type-of", "$value", 0 };
    Node type_of(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node val(bindings[parameters[0]]);
      Node type(Node::string_constant, val.line_number, Token::make());
      type.unquoted = true;
      switch (val.type)
      {
        case Node::number:
        case Node::numeric_dimension:
        case Node::numeric_percentage:
          type.content.token = Token::make(number_name);
          break;
        case Node::boolean:
          type.content.token = Token::make(bool_name);
          break;
        case Node::string_constant:
          type.content.token = Token::make(string_name);
          break;
        case Node::numeric_color:
          type.content.token = Token::make(color_name);
          break;
        case Node::comma_list:
        case Node::space_list:
        case Node::nil:
          type.content.token = Token::make(list_name);
          break;
        default:
          type.content.token = Token::make(string_name);
      }
      return type;
    }
    
    extern const char empty_str[] = "";
    extern const char percent_str[] = "%";
    
    Function_Descriptor unit_descriptor =
    { "unit", "$number", 0 };
    Node unit(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node val(bindings[parameters[0]]);
      Node u(Node::string_constant, val.line_number, Token::make());
      switch (val.type)
      {
        case Node::number:
          u.content.token = Token::make(empty_str);
          break;
        case Node::numeric_percentage:
          u.content.token = Token::make(percent_str);
          break;
        case Node::numeric_dimension:
          u.content.token = Token::make(val.content.dimension.unit, Prelexer::identifier(val.content.dimension.unit));
          break;
        default: // TO DO: throw an exception
          u.content.token = Token::make(empty_str);
          break;
      }
      return u;
    }

    extern const char true_str[]  = "true";
    extern const char false_str[] = "false";

    Function_Descriptor unitless_descriptor =
    { "unitless", "$number", 0 };
    Node unitless(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node val(bindings[parameters[0]]);
      Node result(Node::string_constant, val.line_number, Token::make());
      result.unquoted = true;
      switch (val.type)
      {
        case Node::number: {
          Node T(Node::boolean);
          T.line_number = val.line_number;
          T.content.boolean_value = true;
          return T;
          } break;
        case Node::numeric_percentage:
        case Node::numeric_dimension: {
          Node F(Node::boolean);
          F.line_number = val.line_number;
          F.content.boolean_value = false;
          return F;
          } break;
        default:
          // TO DO: throw an exception;
          break;
      }
      return result;
    }
    
    Function_Descriptor comparable_descriptor =
    { "comparable", "$number_1", "$number_2", 0 };
    Node comparable(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node n1(bindings[parameters[0]]);
      Node n2(bindings[parameters[1]]);
      Node::Type t1 = n1.type;
      Node::Type t2 = n2.type;
      if (t1 == Node::number || t2 == Node::number) {
        Node T(Node::boolean);
        T.line_number = n1.line_number;
        T.content.boolean_value = true;
        return T;
      }
      else if (t1 == Node::numeric_percentage && t2 == Node::numeric_percentage) {
        Node T(Node::boolean);
        T.line_number = n1.line_number;
        T.content.boolean_value = true;
        return T;
      }
      else if (t1 == Node::numeric_dimension && t2 == Node::numeric_dimension) {
        string u1(Token::make(n1.content.dimension.unit, Prelexer::identifier(n1.content.dimension.unit)).to_string());
        string u2(Token::make(n2.content.dimension.unit, Prelexer::identifier(n2.content.dimension.unit)).to_string());
        if (u1 == "ex" && u2 == "ex" ||
            u1 == "em" && u2 == "em" ||
            (u1 == "in" || u1 == "cm" || u1 == "mm" || u1 == "pt" || u1 == "pc") &&
            (u2 == "in" || u2 == "cm" || u2 == "mm" || u2 == "pt" || u2 == "pc")) {
          Node T(Node::boolean);
          T.line_number = n1.line_number;
          T.content.boolean_value = true;
          return T;
        }
        else {
          Node F(Node::boolean);
          F.line_number = n1.line_number;
          F.content.boolean_value = false;
          return F;
        }
      }
      else {
        Node F(Node::boolean);
        F.line_number = n1.line_number;
        F.content.boolean_value = false;
        return F;
      }    
    }
    
    // Boolean Functions ///////////////////////////////////////////////////
    Function_Descriptor not_descriptor =
    { "not", "value", 0 };
    Node not_impl(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node val(bindings[parameters[0]]);
      if (val.type == Node::boolean && val.content.boolean_value == false) {
        Node T(Node::boolean);
        T.line_number = val.line_number;
        T.content.boolean_value = true;
        return T;
      }
      else {
        Node F(Node::boolean);
        F.line_number = val.line_number;
        F.content.boolean_value = false;
        return F;
      }
    }
    
    
  }
}