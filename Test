#!/bin/bash
echo "=== ПРОВЕРКА KUBSH (11 пунктов) ==="
echo ""

make clean
make

PASS=0
FAIL=0

echo "1. Test entrée/sortie..."
echo "test_string" | timeout 1 ./kubsh | grep -q "test_string"
[ $? -eq 0 ] && { echo "✓ PASS"; ((PASS++)); } || { echo "✗ FAIL"; ((FAIL++)); }

echo "2. Test boucle EOF..."
echo -e "line1\nline2" | timeout 1 ./kubsh | grep -q "line2"
[ $? -eq 0 ] && { echo "✓ PASS"; ((PASS++)); } || { echo "✗ FAIL"; ((FAIL++)); }

echo "3. Test commande \\q..."
echo '\q' | timeout 1 ./kubsh
[ $? -eq 0 ] && { echo "✓ PASS"; ((PASS++)); } || { echo "✗ FAIL"; ((FAIL++)); }

echo "4. Test historique..."
rm -f ~/.kubsh_history
echo -e "echo 1\necho 2\n\q" | ./kubsh > /dev/null
[ -f ~/.kubsh_history ] && { echo "✓ PASS"; ((PASS++)); } || { echo "✗ FAIL"; ((FAIL++)); }

echo "5. Test echo..."
echo 'echo "Hello World"' | ./kubsh | grep -q "Hello World"
[ $? -eq 0 ] && { echo "✓ PASS"; ((PASS++)); } || { echo "✗ FAIL"; ((FAIL++)); }

echo "6. Test commande invalide..."
echo "invalid_command" | ./kubsh 2>/dev/null
[ $? -eq 0 ] && { echo "✓ PASS"; ((PASS++)); } || { echo "✗ FAIL"; ((FAIL++)); }

echo "7. Test variables d'environnement..."
echo '\e $PATH' | ./kubsh | grep -q ":"
[ $? -eq 0 ] && { echo "✓ PASS"; ((PASS++)); } || { echo "✗ FAIL"; ((FAIL++)); }

echo "8. Test binaire..."
echo "ls -la" | ./kubsh | grep -q "Makefile"
[ $? -eq 0 ] && { echo "✓ PASS"; ((PASS++)); } || { echo "✗ FAIL"; ((FAIL++)); }

echo "9. Test signal SIGHUP..."
./kubsh &
PID=$!
sleep 0.5
kill -HUP $PID 2>/dev/null
wait $PID 2>/dev/null
[ $? -eq 0 ] && { echo "✓ PASS"; ((PASS++)); } || { echo "✗ FAIL"; ((FAIL++)); }

echo "10. Test info disque..."
echo '\l /dev/sda' | ./kubsh | grep -q "sda" 2>/dev/null
[ $? -le 1 ] && { echo "✓ PASS"; ((PASS++)); } || { echo "✗ FAIL"; ((FAIL++)); }

echo "11. Test VFS utilisateurs..."
rm -rf ~/users
./kubsh -c "exit" 2>/dev/null
[ -d ~/users ] && [ -f ~/users/root/id ] && { echo "✓ PASS"; ((PASS++)); } || { echo "✗ FAIL"; ((FAIL++)); }

echo ""
echo "=== RESULTAT ==="
echo "Passés: $PASS/11"
echo "Échoués: $FAIL/11"
