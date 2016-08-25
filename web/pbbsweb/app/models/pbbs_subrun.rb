class PbbsSubrun < ActiveRecord::Base
    belongs_to :pbbs_run, :foreign_key => "run_id"
    
end
