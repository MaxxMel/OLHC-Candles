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
#include <cmath>

struct Trade
{
    uint64_t local_timestamp; // Время сделки в миллисекундах
    bool side; //(true для покупок, false для продаж)
    double price;
    int amount;
}; 
vector <Trade> Trades; 
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
// служебная 
    uint64_t minTime = 172385279932269189;
    uint64_t maxTime = 0;
class Candle {
    public: 
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
                // служебная информация
                if (::minTime > it.local_timestamp) ::minTime = it.local_timestamp;
                if (::maxTime < it.local_timestamp) ::maxTime = it.local_timestamp; 
            }
        }

        
        if (buy_trade_count > 0) avg_buy_price /= buy_amount;
        if (sell_trade_count > 0) avg_sell_price /= sell_amount;
    }

    void ShowInformation() {
        cout << "Open price: " << open_price << "\n";
        cout << "High price: " << high_price << "\n";
        cout << "Low price: " << low_price << "\n";
        cout << "Close price: " << close_price << "\n";
        cout << "Average buy price: " << avg_buy_price << "\n";
        cout << "Average sell price: " << avg_sell_price << "\n";
        cout << "Buy amount: " << buy_amount << "\n";
        cout << "Sell amount: " << sell_amount << "\n";
    }
  
};
void GetTradeData()
    {
        string instrument =  R"(C:\Users\Maxim\OneDrive\Рабочий стол\CMF\Task1\trades_1000pepeusdt.csv)";
        filesystem::path file_path = instrument;
        filesystem::directory_entry entry(file_path);
    
        ifstream data_file(entry.path()); 
        string line;

        getline(data_file, line);
 
        while (getline(data_file, line))
        {
            Trade T = parseString(line);
            ::Trades.push_back(T);    
        }

    }

//          ---         ---         DANGER ZONE!            ---          ---         ---   DANGER ZONE!      ---         ---     
struct Action {
    Candle& data_for_decision;  // Свеча, используемая для принятия решения
    int operation;  // Количество контрактов, которые нужно купить или продать

    Action(Candle& candle, int op) : data_for_decision(candle), operation(op) {}

    // Покупка/продажа по цене закрытия свечи
    void OpByClosePrice(double& Gpnl) {
        if (operation != 0) {
            Gpnl += operation * data_for_decision.close_price;
        }
    }

    // Покупка/продажа по средней цене покупок/продаж
    void OpByAvgPrice(double& Gpnl) {
        if (operation != 0) {
            double avg_price = operation > 0 ? data_for_decision.avg_buy_price : data_for_decision.avg_sell_price;
            Gpnl += operation * avg_price;
        }
    }
};

struct Strategy {
    vector<Action> actions;
    double pnl = 0.0;  // Финансовый результат
    int position = 0;  // Текущая позиция
    int position_flips = 0;  // Количество переходов позиции через 0
    int total_amount = 0;  // Общий объем торгов

    Strategy(vector<Action>& actions_) : actions(actions_) {}
};

class Simulator {
public:
    vector<Strategy> strategies;  // Вектор стратегий для симуляции

    Simulator(const vector<Strategy>& strategies_) : strategies(strategies_) {}

    // Запуск симулятора в режиме торговли по цене закрытия свечи
    void RunByClosePrice() {
        for (auto& strategy : strategies) {
            for (auto& action : strategy.actions) {
                int previous_position = strategy.position;
                action.OpByClosePrice(strategy.pnl);
                strategy.position += action.operation;
                strategy.total_amount += abs(action.operation);
                if ((previous_position > 0 && strategy.position < 0) || (previous_position < 0 && strategy.position > 0)) {
                    strategy.position_flips++;
                }
            }
        }
    }

    // Запуск симулятора в режиме торговли по средней цене покупок/продаж
    void RunByAvgPrice() {
        for (auto& strategy : strategies) {
            for (auto& action : strategy.actions) {
                int previous_position = strategy.position;
                action.OpByAvgPrice(strategy.pnl);
                strategy.position += action.operation;
                strategy.total_amount += abs(action.operation);
                if ((previous_position > 0 && strategy.position < 0) || (previous_position < 0 && strategy.position > 0)) {
                    strategy.position_flips++;
                }
            }
        }
    }

    // Вывод статистик для каждой стратегии
    void PrintStatistics() {
        for (size_t i = 0; i < strategies.size(); ++i) {
            Strategy& strategy = strategies[i];
            cout << "Strategy " << i + 1 << " Statistics:" << endl;
            cout << "PnL: " << strategy.pnl << endl;
            cout << "Total Traded Volume: " << strategy.total_amount << endl;
            cout << "Position Flips: " << strategy.position_flips << endl;
            // Здесь можно добавить расчет Sharpe ratio, Sortino ratio, max drawdown и других статистик
            cout << endl;
        }
    }

    // Рассчет Sharpe Ratio для стратегии
    double CalculateSharpeRatio(const Strategy& strategy) {
       
        double mean_pnl = strategy.pnl / strategy.actions.size();
        double squared_sum = 0.0;
        for (const auto& action : strategy.actions) {
            squared_sum += pow(action.operation - mean_pnl, 2);
        }
        double stddev_pnl = sqrt(squared_sum / strategy.actions.size());
        return mean_pnl / stddev_pnl;  // Примерный расчет
    }

    // Рассчет Sortino Ratio для стратегии
    double CalculateSortinoRatio(const Strategy& strategy) {
       
        return 0.0; 
    }
};
//          ---         ---         DANGER ZONE!            ---          ---         ---   DANGER ZONE!      ---         ---  

int main() 
{
    
}
