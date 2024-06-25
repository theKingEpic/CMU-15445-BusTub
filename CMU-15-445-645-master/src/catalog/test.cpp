#include "catalog/schema.h"
#include "common/exception.h"
#include "type/type.h"
namespace bustub {
int main() {
  std::vector<Column> columns = {Column("id", TypeId::INTEGER, false), Column("name", TypeId::VARCHAR, true),
                                 Column("age", TypeId::INTEGER, false)};

  Schema schema(columns);

  std::string str = schema.ToString();
  std::cout << str << std::endl;

  return 0;
}
}  // namespace bustub
