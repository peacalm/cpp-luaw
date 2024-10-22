printv = function(v, msg)
  t = {'['}
  for i = 0, v:size() - 1 do
    if i > 0 then
    table.insert(t, #t + 1, ', ')
    end
    table.insert(t, #t + 1, v:at(i))
  end
  table.insert(t, #t + 1, ']')
  if msg ~= nil then
    print(msg, table.concat(t))
  else
    print(table.concat(t))
  end
end

v = NewVD();
assert(v:size() == 0)
assert(v:empty())
for i = 0, 4 do
  v:push_back(i)
end
printv(v, 'init 0~4')

assert(v:size() == 5)
assert(not v:empty())
assert(v:at(2) == 2)

v:pop_back()
assert(v:size() == 4)
printv(v, 'pop back')

v:set_at(2, -1)
assert(v:at(2) == -1)
printv(v, 'v[2] = -1')

v:insert_at(0, 1);
printv(v, 'insert_at(0, 1)')

v:set_at(1, 2)
printv(v, 'v[1] = 2')

v:clear()
assert(v:empty())
printv(v, 'clear')

v:resize(3)
printv(v, 'resize(3)')

print('END')
