/**
 * @file ifpga_namespaces.hpp
 * @author liwei ni (nilw@pcl.ac.cn)  
 * @brief The namespaces manager
 * 
 * @version 0.1
 * @date 2021-06-03
 * @copyright Copyright (c) 2021
 * 
 */

#pragma once

#define iFPGA_NAMESPACE iFPGA

#ifdef iFPGA_NAMESPACE
  #define iFPGA_NAMESPACE_HEADER_START namespace iFPGA_NAMESPACE {
  #define iFPGA_NAMESPACE_HEADER_END };
  
  #define iFPGA_NAMESPACE_USING_NAMESPACE using namespace iFPGA_NAMESPACE;
#endif
