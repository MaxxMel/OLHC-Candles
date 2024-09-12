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
#include <numeric>


struct Trade
{
    uint64_t local_timestamp; // Время сделки в миллисекундах
    bool side; //(true для покупок, false для продаж)
    double price;
    int amount;
}; 

//              ---             ///             ---             // 
class Instrument 
{
    public: 
    vector <Trade> TradesInClass;
    Instrument (vector <Trade>& TradesInClass_) : TradesInClass(TradesInClass_) {}
}; 
//              --              //              --          // 
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
        //
    
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

// A - Avarage(2), C - close(1)
struct Strategy {
    int now = 0 ;
    vector<pair<char, int>> data;  // <Action, Volume>
    vector <Candle> CandleData; 
    // Statistics
    double PnL = 0.0;
    double TradedVolume = 0.0;
    double SharpeRatio = 0.0;
    double SortinoRatio = 0.0;
    double MaxDrawdown = 0.0;
    double avdHoldTime = 0.0;
    int NumOfPosFlips = 0;

    int currentPosition = 0; 
    double currentPnL = 0.0;
    vector<double> pnlHistory;
    int flips = 0;            
    int holdingTime = 0;
    int lastFlipIndex = 0;


    Strategy(vector<pair<char, int>> data_, vector<Candle> CandleData_) : data(data_), CandleData(CandleData_) 
    { if (data.size() != CandleData.size()) exit(2); }

    class iterator {
    public:
        Strategy& parent;
        int current;

        iterator(Strategy& p, int cr) : parent(p), current(cr) {}

        
        iterator operator++() {
            if (current >= parent.data.size()) exit(2);
            current++;
            parent.now++; 
            return *this;
        }

       
        pair<char, int>& operator*() {
            auto& action = parent.data[current];
            char actionType = action.first;
            int volume = action.second;

            
            if (actionType == 'C') {  
                parent.executeTrade(volume, parent.CandleData[parent.now].close_price, current);
                
                //parent.executeTrade(volume, 100.0, current); // СВЯЗАТЬ ЦЕНУ ЗАКРЫТИЯ СО СВЕЧЕЙ
                
            } else if (actionType == 'A') {  
                if (volume < 0) 
                {
                    parent.executeTrade(volume, parent.CandleData[parent.now].avg_sell_price, current);
                }  else {
                parent.executeTrade(volume, parent.CandleData[parent.now].avg_buy_price, current);
                //parent.executeTrade(volume, 99.5, current);  // СВЯЗАТЬ СРЕДНЮЮ ЦЕНУ СО СВЕЧЕЙ
                }
            }

            return parent.data[current];
        }

        bool operator!=(const iterator& other) {
            return current != other.current;
        }
    };

    iterator begin() {
        return iterator(*this, 0);
    }

    iterator end() {
        return iterator(*this, data.size());
    }

    
    void executeTrade(int volume, double price, int& cur) {
        if (volume == 0) return;

        
        double prevPosition = currentPosition;
        currentPosition += volume;
        double tradePnL = volume * price;
        PnL += tradePnL;
        pnlHistory.push_back(PnL);

        
        TradedVolume += abs(volume);

        
        if ((prevPosition > 0 && currentPosition < 0) || (prevPosition < 0 && currentPosition > 0)) {
            flips++;
            NumOfPosFlips = flips;
            holdingTime += cur - lastFlipIndex;
            lastFlipIndex = cur;
        }
    }

   
    void calculateRatios() {
        if (pnlHistory.empty()) return;

        double meanPnL = accumulate(pnlHistory.begin(), pnlHistory.end(), 0.0) / pnlHistory.size();
        double stdDev = sqrt(accumulate(pnlHistory.begin(), pnlHistory.end(), 0.0,
            [meanPnL](double acc, double pnl) {
                return acc + (pnl - meanPnL) * (pnl - meanPnL);
            }) / pnlHistory.size());

        
        SharpeRatio = meanPnL / stdDev;

        
        double downsideDev = sqrt(accumulate(pnlHistory.begin(), pnlHistory.end(), 0.0,
            [meanPnL](double acc, double pnl) {
                return acc + ((pnl < meanPnL) ? (pnl - meanPnL) * (pnl - meanPnL) : 0);
            }) / pnlHistory.size());

        SortinoRatio = meanPnL / downsideDev;

        
        double peakPnL = pnlHistory[0];
        for (const double& pnl : pnlHistory) {
            if (pnl > peakPnL) peakPnL = pnl;
            MaxDrawdown = max(MaxDrawdown, peakPnL - pnl);
        }

        
        avdHoldTime = holdingTime / max(1, flips); 
    }
};


int main() 
{   
    GetTradeData(); 
    Candle C1(1723248002520544, 1723248006297364, Trades); 
    //C1.ShowInformation();
    Candle C2(1723248002520480, 1723471322122114, Trades); 
    //C2.ShowInformation(); 
    vector<Candle> data_for_Strategy = {C1, C2}; 
    vector <pair <char, int > > info = {{'A', 10}, {'C',  -5}}; 

    Strategy myStrategy(info, data_for_Strategy); 
    for (auto& action : myStrategy) {}


    myStrategy.calculateRatios();
    cout << "PnL: " << myStrategy.PnL << endl;
    cout << "Traded Volume: " << myStrategy.TradedVolume << endl;
    cout << "Sharpe Ratio: " << myStrategy.SharpeRatio << endl;
    cout << "Sortino Ratio: " << myStrategy.SortinoRatio << endl;
    cout << "Max Drawdown: " << myStrategy.MaxDrawdown << endl;
    cout << "Average Holding Time: " << myStrategy.avdHoldTime << endl;
    cout << "Position Flips: " << myStrategy.NumOfPosFlips << endl;
    return 0; 
    
}



