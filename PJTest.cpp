// PJTest.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

/*
 * PJLink Projector Emulator
 *
 * File:   main.cpp
 * Author: Alex McLain <alex@alexmclain.com>
 *
 * Website:
 * [TBD]
 *
 * PJLink Specification:
 * http://pjlink.jbmia.or.jp/english/data/PJLink%20Specifications100.pdf
 *
 *
 * ========================================================================
 * Copyright 2012 Alex McLain
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "UI.h"
#include <Windows.h>
int main(int argc, char** argv) {
	UI* ui = UI::getInstance();

	if (argc > 1)
	{
		for (int i = 1; i < argc; ++i) {
			std::string arg = argv[i];
			if (arg == "--hide")
			{
				ShowWindow(GetConsoleWindow(), SW_HIDE);
			}
		}
	}
	
	ui->initialize();
	ui->shutdown();
	return 0;
}

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
