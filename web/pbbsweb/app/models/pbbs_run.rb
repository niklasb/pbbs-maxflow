class PbbsRun < ActiveRecord::Base
    belongs_to :pbbs_problem, :foreign_key => "problem_id"
    belongs_to :pbbs_program, :foreign_key => "program_id"
    belongs_to :pbbs_host, :foreign_key => "host_id"
end
