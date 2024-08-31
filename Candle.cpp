/*
По заданной длине временного окна T
построил open-low-high-close свечи (OLHC candles).
- Каждая свеча соответствует одному периоду времени длины T.
- Симулятор получает параметр - временное окно в миллисекундах в качестве конфигурации
на вход, реализована возможность сгенерировать свечи разной
длины.

- Свеча состоит из начальной, конечной, минимальной, максимальной цены
сделки, средней цены покупок и средней цены продаж; проторгованного объёма на
покупку и на продажу.

*/
using namespace std; 
#include <string>
#include <vector>
#include <deque>
#include <algorithm>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <sstream>


struct Trade
{
    uint64_t local_timestamp; // Время сделки в миллисекундах
    bool side; //(true для покупок, false для продаж)
    double price;
    int amount;
}; 
vector <Trade> Trades; 
Trade parseString(const string& input);
void GetTradeData(int& trades_amount);

Trade parseString(const string& input) {
    Trade trade;
    istringstream iss(input);
    string token;
    if (getline(iss, token, ',')) {
        trade.local_timestamp = stoull(token); 
    }

    
    if (getline(iss, token, ',')) {
        if (token == "buy") {
            trade.side = true;  
        } else {
            trade.side = false; 
        }
    }

    
    if (getline(iss, token, ',')) {
        trade.price = stod(token); 
    }

    if (getline(iss, token, ',')) {
        trade.amount = stoi(token);
    }

    return trade;
}

struct Candle {
    uint64_t open_time;   // Время открытия свечи
    uint64_t duration;    // Длительность свечи
    double open_price;    // Цена открытия
    double high_price;    // Максимальная цена
    double low_price;     // Минимальная цена
    double close_price;   // Цена закрытия
    double avg_buy_price; // Средняя цена покупок
    double avg_sell_price;// Средняя цена продаж
    uint64_t buy_amount;    // Объем покупок
    uint64_t sell_amount;   // Объем продаж

    Candle(uint64_t t, uint64_t d, vector<Trade>& TradeDataBase)
        : open_time(t), duration(d), open_price(0), high_price(0), low_price(0), close_price(0),
          avg_buy_price(0), avg_sell_price(0), buy_amount(0), sell_amount(0) {
        uint64_t close_time = open_time + duration;
        bool first_trade_found = false; 

        int buy_trade_count = 0;
        int sell_trade_count = 0;

        for (const auto& it : TradeDataBase) {
           
            if (it.local_timestamp >= open_time && it.local_timestamp <= close_time) {
                
                if (!first_trade_found) {
                    open_price = it.price;
                    low_price = it.price;  
                    high_price = it.price; 
                    first_trade_found = true;
                }

              
                if (it.price > high_price) high_price = it.price;
                if (it.price < low_price) low_price = it.price;

                
                close_price = it.price;

             
                if (it.side) {
                    // Покупка
                    buy_amount += it.amount;
                    avg_buy_price += it.price * it.amount;
                    buy_trade_count++;
                } else {
                    // Продажа
                    sell_amount += it.amount;
                    avg_sell_price += it.price * it.amount;
                    sell_trade_count++;
                }
            }
        }

        
        if (buy_trade_count > 0) avg_buy_price /= buy_amount;
        if (sell_trade_count > 0) avg_sell_price /= sell_amount;
    }

    void ShowInformation() {
        cout << "Open price: " << open_price << endl;
        cout << "High price: " << high_price << endl;
        cout << "Low price: " << low_price << endl;
        cout << "Close price: " << close_price << endl;
        cout << "Average buy price: " << avg_buy_price << endl;
        cout << "Average sell price: " << avg_sell_price << endl;
        cout << "Buy amount: " << buy_amount << endl;
        cout << "Sell amount: " << sell_amount << endl;
    }
};
void GetTradeData()
    {
        filesystem::path file_path = R"(path.csv)"; // path to dataBase
        filesystem::directory_entry entry(file_path);
    
        ifstream data_file(entry.path()); 
        string line;
        int k(0); 

        while (k < 1 && getline(data_file, line))
        {
            ++k; 
        }
    
        //int trades_amount(0); 
        while (getline(data_file, line))
        {
            Trade T = parseString(line);
            ::Trades.push_back(T);    
        }

    }

int main()
{
    GetTradeData(); 
    Candle C(1723248002520544, 1723248006297364, Trades); 
    C.ShowInformation();
}
