#include "DeclarationAnalyzer.h"

inline bool IsProtectedTypeName(const std::string &name)
{
  return name == "i32" || name == "u32" || name == "isize" || name == "usize" ||
         name == "bool" || name == "Box" || name == "Vec" || name == "Copy" ||
         name == "Clone" || name == "PartialEq" || name == "Eq";
}

inline bool IsProtectedValueName(const std::string &name)
{
  return name == "get_i32" || name == "print_i32" || name == "println_i32";
}

bool DeclarationAnalyzer::DeclCollect(const Crate &crate,
                                      SemanticContext &semantic_ctx)
{
  // 遍历第一遍
  std::vector<const ImplItem *> impl_items;
  for (auto &item : crate.items_)
  {
    if (auto const_item = dynamic_cast<const ConstItem *>(item.get()))
    {
      if (IsProtectedValueName(const_item->ident_))
      {
        return false;
      }
      std::size_t const_id = semantic_ctx.constants_.size();
      auto [it, inserted] = semantic_ctx.func_const_name_to_id.emplace(
          const_item->ident_,
          FuncConstSymbol{.kind_ = ValueKind::Constant, .index_ = const_id});
      if (!inserted)
      {
        return false;
      }
      semantic_ctx.constants_.push_back(
          ConstInfo{.name_ = const_item->ident_, .declaration_ = const_item});
    }
    else if (auto struct_item = dynamic_cast<const StructItem *>(item.get()))
    {
      if (IsProtectedTypeName(struct_item->ident_))
      {
        return false;
      }
      StructId struct_id = semantic_ctx.structs_.size();
      auto [it, inserted] = semantic_ctx.struct_name_to_id_.emplace(
          struct_item->ident_, struct_id);
      if (!inserted)
      {
        return false;
      }
      semantic_ctx.structs_.push_back(
          StructInfo{.name_ = struct_item->ident_,
                     .derives_ = struct_item->derives_,
                     .declaration_ = struct_item});
    }
    else if (auto func_item = dynamic_cast<const FuncItem *>(item.get()))
    {
      if (IsProtectedValueName(func_item->ident_))
      {
        return false;
      }
      std::size_t func_id = semantic_ctx.functions_.size();
      auto [it, inserted] = semantic_ctx.func_const_name_to_id.emplace(
          func_item->ident_,
          FuncConstSymbol{.kind_ = ValueKind::Function, .index_ = func_id});
      if (!inserted)
      {
        return false;
      }
      semantic_ctx.functions_.push_back(
          FuncInfo{.name_ = func_item->ident_, .declaration_ = func_item});
    }
    else if (auto impl_item = dynamic_cast<const ImplItem *>(item.get()))
    {
      impl_items.push_back(impl_item);
    }
  }
  // 遍历第二遍
  for (auto impl_item : impl_items)
  {
    const auto *path_type =
        dynamic_cast<const PathType *>(impl_item->type_.get());
    if (!path_type || path_type->path_segments_.size() != 1)
    {
      return false;
    }
    const auto &segment = path_type->path_segments_.front();
    const std::string &struct_name = segment.name_;
    // 用户定义的结构体不接受类型实参
    if (segment.generic_args_.size() != 0)
    {
      return false;
    }
    // 确定impl的结构体名字已经声明
    if (semantic_ctx.struct_name_to_id_.find(struct_name) ==
        semantic_ctx.struct_name_to_id_.end())
    {
      return false;
    }
    StructId owner_struct_id = semantic_ctx.struct_name_to_id_.at(struct_name);
    StructInfo &owner_struct_info = semantic_ctx.structs_[owner_struct_id];
    for (auto &item : impl_item->items_)
    {
      if (auto const_item = dynamic_cast<const ConstItem *>(item.get()))
      {
        // 检查并插入struct_info里面的映射关系
        std::size_t new_id = semantic_ctx.constants_.size();
        auto [it, inserted] = owner_struct_info.associate_items_.emplace(
            const_item->ident_,
            FuncConstSymbol{.kind_ = ValueKind::Constant, .index_ = new_id});
        if (!inserted) // 重名
        {
          return false;
        }
        semantic_ctx.constants_.push_back(
            ConstInfo{.name_ = const_item->ident_,
                      .owner_struct_id_ = owner_struct_id,
                      .declaration_ = const_item});
      }
      else if (auto func_item = dynamic_cast<const FuncItem *>(item.get()))
      {
        // 检查并插入struct_info里面的映射关系
        std::size_t new_id = semantic_ctx.functions_.size();
        auto [it, inserted] = owner_struct_info.associate_items_.emplace(
            func_item->ident_,
            FuncConstSymbol{.kind_ = ValueKind::Function, .index_ = new_id});
        if (!inserted) // 重名
        {
          return false;
        }
        semantic_ctx.functions_.push_back(
            FuncInfo{.name_ = func_item->ident_,
                      .owner_struct_id_ = owner_struct_id,
                      .declaration_ = func_item});
      }
    }
  }
  return true;
}
