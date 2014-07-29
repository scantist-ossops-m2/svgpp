// Copyright Oleg Maximenko 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://github.com/svgpp/svgpp for library home page.

#pragma once

// libxml2 headers must be already included by user

#include <svgpp/definitions.hpp>
#include <svgpp/detail/namespace.hpp>
#include <svgpp/policy/xml_fwd.hpp>
#include <boost/noncopyable.hpp>
#include <boost/range/as_literal.hpp>
#include <boost/range/iterator_range.hpp>

namespace svgpp
{

namespace detail
{
  class libxml_string_ptr: boost::noncopyable
  {
  public:
    explicit libxml_string_ptr(xmlChar * str)
      : str_(str)
    {}

    ~libxml_string_ptr()
    {
      xmlFree(str_);
    }

    const char * c_str() const
    {
      return reinterpret_cast<const char *>(str_);
    }

  private:
    xmlChar * str_;
  };
}
  
namespace policy { namespace xml
{

template<>
struct attribute_iterator<xmlAttr *>
{
  typedef xmlAttr * iterator_type;
  typedef boost::iterator_range<char const *> string_type;
  typedef string_type attribute_name_type;
  typedef string_type attribute_value_type;
  typedef xmlAttr * saved_value_type;

  static string_type get_string_range(string_type const & str)
  { 
    return str;
  }

  static void advance(iterator_type & xml_attribute)
  {
    xml_attribute = xml_attribute->next;
  }

  static bool is_end(iterator_type xml_attribute)
  {
    return xml_attribute == NULL;
  }

  static BOOST_SCOPED_ENUM(detail::namespace_id) get_namespace(iterator_type xml_attribute)
  {
    if (xml_attribute->ns == NULL)
      return detail::namespace_id::svg;
    boost::iterator_range<char const *> ns_uri = boost::as_literal(reinterpret_cast<const char *>(xml_attribute->ns->href));
    if (boost::range::equal(detail::xml_namespace_uri<char>(), ns_uri))
      return detail::namespace_id::xml;
    else if (boost::range::equal(detail::xlink_namespace_uri<char>(), ns_uri))
      return detail::namespace_id::xlink;
    return detail::namespace_id::other;
  }

  static attribute_name_type get_local_name(iterator_type xml_attribute)
  {
    int len;
    const xmlChar *	local_name = xmlSplitQName3(xml_attribute->name, &len);
    if (!local_name)
      local_name = xml_attribute->name;
    return boost::as_literal(reinterpret_cast<const char *>(local_name));
  }

  static attribute_value_type get_value(iterator_type xml_attribute)
  {
    return boost::as_literal(
      detail::libxml_string_ptr(xmlNodeListGetString(xml_attribute->doc, xml_attribute->children, 1)).c_str());
  }

  static saved_value_type save_value(iterator_type xml_attribute)
  {
    return xml_attribute;
  }
};

template<>
struct element_iterator<xmlNode *>
{
  typedef xmlNode * iterator_type;
  typedef boost::iterator_range<char const *> string_type;
  typedef boost::iterator_range<char const *> element_name_type;
  typedef boost::iterator_range<char const *> element_text_type;
  typedef xmlAttr * attribute_enumerator_type;

  static string_type get_string_range(string_type const & str)
  { 
    return str;
  }

  static void advance_element(iterator_type & xml_element)
  {
    xml_element = xml_element->next; 
    find_next<false>(xml_element);
  }

  static void advance_element_or_text(iterator_type & xml_element)
  {
    xml_element = xml_element->next; 
    find_next<true>(xml_element);
  }

  static bool is_end(iterator_type xml_element)
  {
    return xml_element == NULL;
  }

  static bool is_text(iterator_type xml_element)
  {
    return xml_element->type != XML_ELEMENT_NODE;
  }

  static element_name_type get_local_name(iterator_type xml_element)
  {
    int len;
    const xmlChar *	local_name = xmlSplitQName3(xml_element->name, &len);
    if (!local_name)
      local_name = xml_element->name;
    return boost::as_literal(reinterpret_cast<const char *>(local_name));
  }

  static element_text_type get_text(iterator_type xml_element)
  {
    return boost::as_literal(
      detail::libxml_string_ptr(xmlNodeListGetString(xml_element->doc, xml_element, 1)).c_str());
  }

  static attribute_enumerator_type get_attributes(iterator_type xml_element)
  {
    return xml_element->properties;
  }

  static iterator_type get_child_elements(iterator_type xml_element)
  {
    iterator_type child_element = xml_element->children;
    find_next<false>(child_element);
    return child_element;
  }

  static iterator_type get_child_elements_and_texts(iterator_type xml_element)
  {
    iterator_type child_element = xml_element->children;
    find_next<true>(child_element);
    return child_element;
  }

private:
  template<bool TextsAlso>
  static void find_next(iterator_type & xml_element)
  {
    // TODO: optimize namespace checking by saving pointer to last namespace known to be SVG
    for(; xml_element; xml_element = xml_element->next)
    {
      switch(xml_element->type)
      {
      case XML_ELEMENT_NODE:
      {
        if (xml_element->ns == NULL)
          break;
        boost::iterator_range<char const *> ns_uri = boost::as_literal(reinterpret_cast<const char *>(xml_element->ns->href));
        if (boost::range::equal(detail::svg_namespace_uri<char>(), ns_uri))
          return;
        break;
      }
      case XML_TEXT_NODE:
      case XML_CDATA_SECTION_NODE:
        if (TextsAlso)
          return;
        break;
      }
    }
  }
};

}}}