int GetCPUID()
{
  int id;

  asm(
  "mfc0 %0, $22\n"
  : "=r"(id)
  );

  return id;
}