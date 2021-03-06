#ifndef GRAPHMOD_TRUNCATED_BETA_BERNOULLI_FACTOR_HPP
#define GRAPHMOD_TRUNCATED_BETA_BERNOULLI_FACTOR_HPP

#include <functional>
#include "factor.hpp"
#include "variable_interface.hpp"
#include "categorical_variable.hpp"
#include "mapped_categorical_variable.hpp"
#include "continuous_matrix_variable.hpp"
#include "probability_vector.hpp"

namespace graphmod{
  /*
    our convention is to associate the first parameter (alpha) with true values
   */

  template<class counts_type>
  class TruncatedBetaBernoulliFactor : public Factor<TruncatedBetaBernoulliFactor<counts_type>, counts_type>{
  public:
    TruncatedBetaBernoulliFactor(ContinuousMatrixVariable<counts_type>* prior, 
			CategoricalVariable<counts_type>* index, 
			MappedCategoricalVariable<counts_type>* observation) : 
      Factor<TruncatedBetaBernoulliFactor<counts_type>, counts_type>({prior, index}, {observation}),
      _prior(prior), 
      _index(index), 
      _observation(observation)
    {
      prior->add_child(this);
      index->add_child(this);
      observation->add_parent(this);
    }

    TruncatedBetaBernoulliFactor(){
    }

    virtual FactorInterface<counts_type>* clone(std::map<VariableInterface<counts_type>*, VariableInterface<counts_type>*>& old_to_new) const{
      return new TruncatedBetaBernoulliFactor(
					      dynamic_cast<ContinuousMatrixVariable<counts_type>*>(old_to_new[_prior]), 
					      dynamic_cast<CategoricalVariable<counts_type>*>(old_to_new[_index]), 
					      dynamic_cast<MappedCategoricalVariable<counts_type>*>(old_to_new[_observation])
					      );
    }

    virtual ~TruncatedBetaBernoulliFactor(){
    }

    virtual std::string type() const{
      return "TruncatedBetaBernoulli";
    }

    void compile_implementation(counts_type& counts) const{
      if(_observation->get_domain_size() != _prior->get_value().size()){
	double value = _prior->get_value()[0][0];
	_prior->get_value().resize(2);
	_prior->get_value()[0].resize(_observation->get_domain_size());
	_prior->get_value()[1].resize(_observation->get_domain_size());
	std::fill(_prior->get_value()[0].begin(), _prior->get_value()[0].end(), value);
	std::fill(_prior->get_value()[1].begin(), _prior->get_value()[1].end(), value);
      }
      counts.add_target({_index->get_domain_name(), _observation->get_domain_name()}, {_index->get_domain_size(), _observation->get_domain_size()});
      counts.add_target({_index->get_domain_name()}, {_index->get_domain_size()});
    }

    static inline double log_density_function(std::vector<std::vector<double> > priors, int index_total, std::vector<int> index_observation_counts, std::map<int, bool> observations){
      return std::accumulate(observations.begin(), observations.end(), 0.0, [&](double current_log, std::pair<int, bool> observation){
	  int obs_id = observation.first;
	  int ind_obs_count = index_observation_counts[obs_id];
	  return current_log + std::log((ind_obs_count + priors[0][obs_id]) / (index_total + priors[0][obs_id] + priors[1][obs_id]));
	});
    }

    double log_density_implementation(counts_type& counts) const{
      return log_densities_implementation(counts, _index).at(_index->get_value());
    }

    LogProbabilityVector log_densities_implementation(counts_type& counts, const VariableInterface<counts_type>* variable) const{
      std::vector<double> log_probs;
      if(variable == _index){
	std::string index_domain_name = _index->get_domain_name(), observation_domain_name = _observation->get_domain_name();
	auto index_by_observation = counts(index_domain_name, observation_domain_name);
	std::vector<int> index_totals;
	std::transform(index_by_observation.begin(), index_by_observation.end(), std::back_inserter(index_totals), 
		       [](std::vector<int>& row){ 
			 return std::accumulate(row.begin(), row.end(), 0);
		       });
	auto& prior_values = _prior->get_value();
	auto& observation_values = _observation->get_value();
	std::transform(index_by_observation.begin(), index_by_observation.end(), index_totals.begin(), std::back_inserter(log_probs),
		       [this,&prior_values,&observation_values](std::vector<int>& counts, int index_total){
			 return log_density_function(prior_values, index_total, counts, observation_values);
		       });
      }
      else{
	throw GraphmodException("unimplemented: vector density for TruncatedBetaBernoulli over observation");
      }
      return LogProbabilityVector(log_probs);
    }
    
    void adjust_counts_implementation(counts_type& counts, int weight) const{      

      int index_value = _index->get_value();
      if(index_value != -1){
	std::string index_domain_name = _index->get_domain_name(), obs_domain_name = _observation->get_domain_name();
	counts.increment({index_domain_name}, {index_value}, weight);
	for(auto opair: _observation->get_value()){
	  counts.increment({index_domain_name, obs_domain_name}, {index_value, opair.first}, weight);
	}
      }

    }
  private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version){
      ar & boost::serialization::base_object<Factor<TruncatedBetaBernoulliFactor<counts_type>, counts_type> >(*this);
      ar & _prior;
      ar & _observation;
      ar & _index;
    }
    ContinuousMatrixVariable<counts_type>* _prior;
    CategoricalVariable<counts_type>* _index;
    MappedCategoricalVariable<counts_type>* _observation;
  };
}

#endif
