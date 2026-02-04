class Flight:
    def init(self, origin: str, dest: str, day: int, price: int):
        self.origin = origin
        self.dest = dest
        self.day = day
        self.price = price
    
    def repr(self):
        return f"{self.origin} {self.dest} {self.day} {self.price}"

class CSPFlightPlanner:
    def init(self):
        self.flights_by_route: Dict[Tuple[str, str], List[Flight]] = {}
        self.variables: List[int] = []
        self.domains: List[List[Flight]] = []
        self.min_price: int = 0
        self.max_price: int = 0
        self.cities: List[str] = []
        self.stay_ranges: List[Tuple[int, int]] = []
        self.assignment: List[Optional[Flight]] = []
    
    def read_input(self):
        M = int(input().strip())
        self.min_price, self.max_price = map(int, input().strip().split())
        self.cities = input().strip().split()
        
        stay_values = list(map(int, input().strip().split()))
        self.stay_ranges = []
        for i in range(0, len(stay_values), 2):
            self.stay_ranges.append((stay_values[i], stay_values[i+1]))
        
        for _ in range(M):
            origin, dest, day, price = input().strip().split()
            flight = Flight(origin, dest, int(day), int(price))
            
            route = (origin, dest)
            if route not in self.flights_by_route:
                self.flights_by_route[route] = []
            self.flights_by_route[route].append(flight)
        
        self._create_variables_and_domains()
    
    def _create_variables_and_domains(self):
        self.variables = list(range(len(self.cities) - 1))
        self.domains = []
        self.assignment = [None] * len(self.variables)
        
        for i in range(len(self.cities) - 1):
            origin = self.cities[i]
            dest = self.cities[i + 1]
            route = (origin, dest)
            
            if route in self.flights_by_route:
                flights = sorted(self.flights_by_route[route], key=lambda f: f.day)
                self.domains.append(flights.copy())
            else:
                self.domains.append([])
    
    def forward_checking(self, var_index: int, assigned_flight: Flight) -> bool:
        if var_index >= len(self.variables) - 1:
            return True
        
        next_var = var_index + 1
        next_domain = self.domains[next_var]
        new_domain = []
        
        assigned_day = assigned_flight.day
        min_stay, max_stay = self.stay_ranges[var_index]
        
        for flight in next_domain:
            days_diff = flight.day - assigned_day
            if min_stay <= days_diff <= max_stay:
                new_domain.append(flight)
        
        self.domains[next_var] = new_domain
        return len(new_domain) > 0
    
    def check_budget_constraint(self) -> bool:
        if None in self.assignment:
            return True
        
        total_cost = sum(flight.price for flight in self.assignment)
        return self.min_price <= total_cost <= self.max_price